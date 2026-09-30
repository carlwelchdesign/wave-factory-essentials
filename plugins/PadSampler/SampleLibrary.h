#pragma once
#include "SampleLoader.h"
#include "ClipTrim.h"
#include "PadFX.h"
#include "json.hpp"
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <thread>

namespace padsampler {
using Json = nlohmann::json;
constexpr int kSlotParams = 9, kLegacyParameterCount = kSlots * kSlotParams + 2;
constexpr int kParameterCount = kFxFirstParam + kSlots * kFxPerPad;
constexpr std::array<std::pair<double,double>,kSlotParams> kBaseBounds{{
  {-60,12},{-1,1},{0,1},{20,20000},{20,20000},{.25,4},{0,1},{0,127},{0,16}
}};
inline FxChainExchange::Bank DocumentFX(const Json& doc) {
  FxChainExchange::Bank bank;
  if(doc.value("version",0)<4)return bank;
  for(int i=0;i<kSlots;++i){
    const auto& effects=doc["slots"][i]["effects"];
    if(!effects.is_array() || effects.size()>3)throw std::runtime_error("Invalid FX rack");
    for(const auto& name:effects){
      if(!name.is_string())throw std::runtime_error("Invalid FX type");
      int type=0;for(;type<kFxTypes;++type)if(name.get<std::string>()==kFxNames[type])break;
      if(type==kFxTypes || !bank[i].Add(FxType(type)))throw std::runtime_error("FX rack requires distinct supported effects");
    }
  }
  return bank;
}
inline Json FXDocument(const FxChainExchange::Bank& bank) {
  Json slots=Json::array();for(const auto& chain:bank){Json effects=Json::array();for(int i=0;i<chain.count;++i)effects.push_back(kFxNames[int(chain.order[i])]);slots.push_back(effects);}return slots;
}
inline CurveExchange::Bank DocumentCurves(const Json& doc) {
  CurveExchange::Bank bank;
  if(doc.value("version",0)==1) { for(auto& c:bank) c.legacy=true; return bank; }
  if(!doc.contains("curves") || !doc["curves"].is_array() || doc["curves"].size()!=6) throw std::runtime_error("Invalid tone curves");
  for(int i=0;i<6;++i) {
    const auto& c=doc["curves"][i];
    if(!c.is_object() || !c.contains("legacy") || !c["legacy"].is_boolean() || !c.contains("points") || !c["points"].is_array() || c["points"].size()<2 || c["points"].size()>8) throw std::runtime_error("Invalid tone curve points");
    bank[i].legacy=c["legacy"].get<bool>(); bank[i].count=int(c["points"].size());
    for(int j=0;j<bank[i].count;++j) {
      const auto& p=c["points"][j];
      if(!p.is_array() || p.size()!=2 || !p[0].is_number() || !p[1].is_number()) throw std::runtime_error("Invalid tone curve coordinate");
      bank[i].points[j]={p[0].get<double>(),p[1].get<double>()};
    }
    if(!bank[i].Valid()) throw std::runtime_error("Tone curve must have ordered points and rising brightness");
  }
  return bank;
}
inline Json CurveDocument(const CurveExchange::Bank& bank) {
  Json result=Json::array();
  for(const auto& c:bank) { Json points=Json::array(); for(int i=0;i<c.count;++i) points.push_back({c.points[i].x,c.points[i].y}); result.push_back({{"legacy",c.legacy},{"points",points}}); }
  return result;
}
struct SlotView {
  std::string name, path, status = "Drop WAV / AIFF or choose Load";
  std::array<float, 128> waveform{};
  ClipTrim clip;
  double rate = 0.;
  size_t frames = 0;
  bool ready = false;
  uint64_t generation = 0;
};
struct LibraryView { std::array<SlotView, kSlots> slots; std::string message; };
inline ClipTrim DocumentTrim(const Json& doc, int slot) {
  if (doc.value("version", 0) < 3) return {};
  const auto& value = doc["slots"][slot]["clip"];
  ClipTrim trim;
  trim.start = value["start"].get<double>();
  if (!value["end"].is_null()) trim.end = value["end"].get<double>();
  return trim;
}
inline void ValidateDocument(const Json& doc) {
  if (!doc.is_object() || !doc.contains("version") || !doc["version"].is_number_integer() || doc["version"].get<int>()<1 || doc["version"].get<int>()>4 || !doc.contains("slots") || !doc["slots"].is_array() || doc["slots"].size() != kSlots || !doc.contains("parameters") || !doc["parameters"].is_array() || doc["parameters"].size() != (doc["version"].get<int>()==4?kParameterCount:kLegacyParameterCount)) throw std::runtime_error("Unsupported or malformed PadSampler kit/state");
  for (const auto& s : doc["slots"]) {
    if (!s.is_object() || !s.contains("name") || !s["name"].is_string() || s["name"].get<std::string>().size() > 64 || !s.contains("path") || !s["path"].is_string() || s["path"].get<std::string>().size() > 4096) throw std::runtime_error("Invalid slot name or sample path");
    if (doc["version"].get<int>() >= 3) {
      if (!s.contains("clip") || !s["clip"].is_object() || !s["clip"].contains("start") || !s["clip"]["start"].is_number() || !s["clip"].contains("end") || (!s["clip"]["end"].is_null() && !s["clip"]["end"].is_number())) throw std::runtime_error("Invalid clip trim document");
      ClipTrim trim{s["clip"]["start"].get<double>(), s["clip"]["end"].is_null() ? std::optional<double>{} : std::optional<double>{s["clip"]["end"].get<double>()}};
      if (!trim.Valid()) throw std::runtime_error("Clip trim requires ordered times within 60 seconds");
    }
  }
  DocumentCurves(doc);
  DocumentFX(doc);
  for (const auto& v : doc["parameters"]) if (!v.is_number() || !std::isfinite(v.get<double>())) throw std::runtime_error("Invalid parameter value");
  if(doc["version"].get<int>()==4){
    for(int pad=0;pad<kSlots;++pad){
      for(int i=0;i<kSlotParams;++i){double value=doc["parameters"][pad*kSlotParams+i].get<double>();
        if(value<kBaseBounds[i].first || value>kBaseBounds[i].second || ((i==6 || i==7 || i==8) && value!=std::floor(value)))throw std::runtime_error("Pad parameter outside its supported range");
      }
      for(int i=0;i<kFxPerPad;++i){double value=doc["parameters"][kFxFirstParam+pad*kFxPerPad+i].get<double>();
        if(value<kFxBounds[i].first || value>kFxBounds[i].second || ((i==4 || i==9 || i==16 || i==23 || i==28) && value!=0. && value!=1.))throw std::runtime_error("FX parameter outside its supported range");
      }
    }
    double audition=doc["parameters"][54].get<double>(),master=doc["parameters"][55].get<double>();
    if(audition<1 || audition>127 || audition!=std::floor(audition) || master< -60 || master>0)throw std::runtime_error("Global parameter outside its supported range");
  }
}
class SampleLibrary {
 public:
  using Publisher = std::function<bool(int, Sample*)>;
  explicit SampleLibrary(Publisher publisher, std::function<void()> shutdown) : publisher_(std::move(publisher)), shutdown_(std::move(shutdown)), loader_(MakeAppleSampleLoader()) {
    const char* names[] = {"Pad 1", "Pad 2", "Pad 3", "Pad 4", "External Tip", "External Ring"};
    for (int i = 0; i < kSlots; ++i) view_.slots[i].name = names[i];
    worker_ = std::thread([this] { Run(); });
  }
  ~SampleLibrary() { Stop(); shutdown_(); for (auto* sample : current_) if (sample) sample->Release(); }
  void Stop() { { std::lock_guard<std::mutex> lock(mutex_); stopping_ = true; } wake_.notify_one(); if (worker_.joinable()) worker_.join(); }
  LibraryView View() const { std::lock_guard<std::mutex> lock(mutex_); return view_; }
  void Rename(int slot, const std::string& name) { std::lock_guard<std::mutex> lock(mutex_); view_.slots[slot].name = name.substr(0, 64); }
  void Message(const std::string& text) { std::lock_guard<std::mutex> lock(mutex_); view_.message = text; }
  void Clear(int slot) {
    Enqueue([this, slot] {
      if (!publisher_(slot, nullptr)) { Message("Audio update queue full; start playback and retry"); return; }
      if (current_[slot]) { current_[slot]->Release(); current_[slot] = nullptr; }
      std::lock_guard<std::mutex> lock(mutex_);
      auto name = view_.slots[slot].name; view_.slots[slot] = SlotView{}; view_.slots[slot].name = name;
    });
  }
  void Load(int slot, std::string path) {
    { std::lock_guard<std::mutex> lock(mutex_);
      if (stopping_ || jobs_.size() >= 32) { view_.message = "Sample loader is busy; retry shortly"; return; }
      ++view_.slots[slot].generation;
      view_.slots[slot].ready = false;
      jobs_.push_back([this, slot, path] { LoadNow(slot, path, false); });
    }
    wake_.notify_one();
  }
  bool SetTrim(int slot, ClipTrim trim) {
    if (slot < 0 || slot >= kSlots || !trim.Valid()) return false;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto& view = view_.slots[slot];
      if (stopping_ || jobs_.size() >= 32 || !view.ready || view.frames < 2) return false;
      const auto range = ResolveTrim(trim, view.rate, view.frames);
      if (range.reset) return false;
      trim = FramesToTrim(range.start, range.end, view.rate, view.frames);
      view.clip = trim;
      const auto generation = view.generation;
      jobs_.push_back([this, slot, range, generation] {
        { std::lock_guard<std::mutex> lock(mutex_); if (view_.slots[slot].generation != generation) return; }
        if (current_[slot]) current_[slot]->SetTrim(range.start, range.end);
      });
    }
    wake_.notify_one(); return true;
  }
  Json Document(const std::vector<double>& parameters, const CurveExchange::Bank& curves = {}, const FxChainExchange::Bank& fx = {}) const {
    return ComposeDocument(View(), parameters, curves, fx);
  }
  static Json ComposeDocument(const LibraryView& view, const std::vector<double>& parameters, const CurveExchange::Bank& curves, const FxChainExchange::Bank& fx = {}) {
    Json slots = Json::array();
    auto racks=FXDocument(fx);
    for (int i=0;i<kSlots;++i){auto& s=view.slots[i];slots.push_back({{"name", s.name}, {"path", s.path}, {"clip", {{"start", s.clip.start}, {"end", s.clip.end ? Json(*s.clip.end) : Json(nullptr)}}}, {"effects",racks[i]}});}
    return {{"version", 4}, {"parameters", parameters}, {"slots", slots}, {"curves", CurveDocument(curves)}};
  }
  void Restore(Json doc) {
    ValidateDocument(doc);
    { std::lock_guard<std::mutex> lock(mutex_);
      if (stopping_ || jobs_.size() >= 32) { view_.message = "Sample loader is busy; retry session restore"; return; }
      for (int i = 0; i < kSlots; ++i) { view_.slots[i].name = doc["slots"][i]["name"].get<std::string>(); view_.slots[i].path = doc["slots"][i]["path"].get<std::string>(); view_.slots[i].clip = DocumentTrim(doc, i); view_.slots[i].ready = false; ++view_.slots[i].generation; }
      jobs_.push_back([this, doc] { RestoreNow(doc); });
    }
    wake_.notify_one();
  }
  void OpenKit(std::string path) {
    Enqueue([this, path] {
      try {
        if (std::filesystem::file_size(path) > 65536) throw std::runtime_error("Kit manifest is too large");
        std::ifstream input(path); Json doc; input >> doc; ValidateDocument(doc);
        auto parent = std::filesystem::path(path).parent_path();
        for (auto& slot : doc["slots"]) {
          std::string relative = slot["path"];
          if (relative.empty()) continue;
          auto p = std::filesystem::path(relative);
          if (p.is_absolute()) throw std::runtime_error("Portable kit contains an absolute sample path");
          for (const auto& part : p) if (part == "..") throw std::runtime_error("Portable kit path leaves its directory");
          slot["path"] = (parent / p).string();
        }
        RestoreNow(doc);
        { std::lock_guard<std::mutex> lock(mutex_); ready_ = doc; }
        Message("Kit opened. Any missing samples can be relinked with Load.");
      } catch (const std::exception& e) { Message(e.what()); }
    });
  }
  bool TakeReady(Json& doc) { std::lock_guard<std::mutex> lock(mutex_); if (ready_.is_null()) return false; doc = std::move(ready_); ready_ = nullptr; return true; }
  void SaveKit(std::string path, std::vector<double> parameters, CurveExchange::Bank curves = {}, FxChainExchange::Bank fx = {}) {
    auto view = View(); auto snapshot = ComposeDocument(view, parameters, curves, fx);
    Enqueue([this, path, snapshot, view] {
      namespace fs = std::filesystem;
      fs::path target(path), temporary; bool ownTemporary = false;
      try {
        if (target.extension() != ".padkit") target += ".padkit";
        if (fs::exists(target)) throw std::runtime_error("Choose a new kit name; existing kits are never overwritten");
        temporary = target.string() + ".partial";
        if (!fs::create_directory(temporary)) throw std::runtime_error("Kit staging folder already exists; choose another name");
        ownTemporary = true;
        fs::create_directory(temporary / "Samples");
        auto doc = snapshot;
        { std::lock_guard<std::mutex> lock(mutex_);
          for (int i = 0; i < kSlots; ++i) if (view_.slots[i].generation != view.slots[i].generation || view_.slots[i].ready != view.slots[i].ready)
            throw std::runtime_error("Samples changed while saving; retry Save Kit");
        }
        for (int i = 0; i < kSlots; ++i) {
          if (current_[i]) {
            std::string relative = "Samples/slot-" + std::to_string(i + 1) + ".wav";
            loader_->WriteWave((temporary / relative).string(), *current_[i]);
            doc["slots"][i]["path"] = relative;
          } else if (!doc["slots"][i]["path"].get<std::string>().empty()) {
            throw std::runtime_error("Relink missing samples before collecting this kit");
          }
        }
        std::ofstream out(temporary / "kit.json"); out << doc.dump(2); out.close();
        if (!out) throw std::runtime_error("Cannot write kit manifest");
        fs::rename(temporary, target); temporary.clear();
        Message("Saved portable kit: " + target.string());
      } catch (const std::exception& e) {
        if (ownTemporary && !temporary.empty()) { std::error_code ec; fs::remove_all(temporary, ec); }
        Message(e.what());
      }
    });
  }
 private:
  void Enqueue(std::function<void()> job) {
    { std::lock_guard<std::mutex> lock(mutex_);
      if (stopping_) return;
      if (jobs_.size() >= 32) { view_.message = "Sample loader is busy; retry shortly"; return; }
      jobs_.push_back(std::move(job));
    }
    wake_.notify_one();
  }
  void Collect() {
    assets_.erase(std::remove_if(assets_.begin(), assets_.end(), [](auto& sample) { return sample->references.load(std::memory_order_acquire) == 0; }), assets_.end());
  }
  size_t Memory() const { size_t bytes = 0; for (auto& sample : assets_) bytes += sample->data.size() * sizeof(float); return bytes; }
  void LoadNow(int slot, const std::string& path, bool restoring) {
    { std::lock_guard<std::mutex> lock(mutex_); view_.slots[slot].status = "Loading…"; }
    try {
      Collect();
      auto sample = loader_->Load(path, kMemoryLimit - std::min(kMemoryLimit, Memory()));
      SlotView update;
      update.path = path; update.status = "Ready • " + std::to_string(sample->channels) + " ch";
      update.rate = sample->rate; update.frames = sample->Frames(); update.ready = true;
      const bool relinking = restoring || !current_[slot];
      if (relinking) {
        std::lock_guard<std::mutex> lock(mutex_);
        update.clip = view_.slots[slot].clip;
      }
      const auto range = ResolveTrim(update.clip, sample->rate, sample->Frames());
      if (range.reset) { update.clip = {}; update.status += " • Saved trim exceeded this sample; reset to full length"; }
      else update.clip = FramesToTrim(range.start, range.end, sample->rate, sample->Frames());
      sample->SetTrim(range.start, range.end);
      for (size_t i = 0; i < sample->Frames(); ++i) {
        size_t bin = std::min<size_t>(127, i * 128 / sample->Frames());
        for (int c = 0; c < sample->channels; ++c) update.waveform[bin] = std::max(update.waveform[bin], std::abs(sample->data[i * sample->channels + c]));
      }
      // Retain in library before publishing; vector allocation cannot strand an audio reference.
      auto* raw = sample.get(); raw->Retain(); assets_.push_back(std::move(sample));
      if (!publisher_(slot, raw)) { raw->Release(); throw std::runtime_error("Audio update queue full; start playback and retry"); }
      if (current_[slot]) current_[slot]->Release();
      current_[slot] = raw;
      { std::lock_guard<std::mutex> lock(mutex_); update.name = view_.slots[slot].name; update.generation = view_.slots[slot].generation; view_.slots[slot] = update; }
    } catch (const std::exception& e) {
      std::lock_guard<std::mutex> lock(mutex_);
      view_.slots[slot].status = e.what();
      view_.slots[slot].ready = current_[slot] != nullptr;
      if (restoring) view_.slots[slot].path = path;
    }
  }
  void RestoreNow(const Json& doc) {
    for (int i = 0; i < kSlots; ++i) {
      if (!publisher_(i, nullptr)) { Message("Audio update queue full; start playback and reopen kit"); return; }
      if (current_[i]) { current_[i]->Release(); current_[i] = nullptr; }
      { std::lock_guard<std::mutex> lock(mutex_); auto generation = view_.slots[i].generation + 1; view_.slots[i] = SlotView{}; view_.slots[i].generation = generation; view_.slots[i].name = doc["slots"][i]["name"].get<std::string>(); view_.slots[i].path = doc["slots"][i]["path"].get<std::string>(); view_.slots[i].clip = DocumentTrim(doc, i); }
      auto path = doc["slots"][i]["path"].get<std::string>();
      if (!path.empty()) LoadNow(i, path, true);
    }
  }
  void Run() {
    for (;;) {
      std::function<void()> job;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        wake_.wait_for(lock, std::chrono::milliseconds(100), [&] { return stopping_ || !jobs_.empty(); });
        if (stopping_) break;
        if (!jobs_.empty()) { job = std::move(jobs_.front()); jobs_.pop_front(); }
      }
      try { if (job) job(); Collect(); } catch (const std::exception& e) { Message(e.what()); }
    }
  }
  Publisher publisher_;
  std::function<void()> shutdown_;
  std::unique_ptr<SampleLoader> loader_;
  std::vector<std::unique_ptr<Sample>> assets_;
  std::array<Sample*, kSlots> current_{};
  mutable std::mutex mutex_;
  std::condition_variable wake_;
  std::deque<std::function<void()>> jobs_;
  LibraryView view_;
  Json ready_;
  bool stopping_ = false;
  std::thread worker_;
};
}
