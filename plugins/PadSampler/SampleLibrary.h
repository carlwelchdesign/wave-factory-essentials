#pragma once
#include "SampleLoader.h"
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
constexpr int kSlotParams = 9, kParameterCount = kSlots * kSlotParams + 2;
struct SlotView {
  std::string name, path, status = "Drop WAV / AIFF or choose Load";
  std::array<float, 128> waveform{};
};
struct LibraryView { std::array<SlotView, kSlots> slots; std::string message; };
inline void ValidateDocument(const Json& doc) {
  if (!doc.is_object() || doc.value("version", 0) != 1 || !doc.contains("slots") || !doc["slots"].is_array() || doc["slots"].size() != kSlots || !doc.contains("parameters") || !doc["parameters"].is_array() || doc["parameters"].size() != kParameterCount) throw std::runtime_error("Unsupported or malformed PadSampler kit/state");
  for (const auto& s : doc["slots"]) {
    if (!s.is_object() || !s.contains("name") || !s["name"].is_string() || s["name"].get<std::string>().size() > 64 || !s.contains("path") || !s["path"].is_string() || s["path"].get<std::string>().size() > 4096) throw std::runtime_error("Invalid slot name or sample path");
  }
  for (const auto& v : doc["parameters"]) if (!v.is_number() || !std::isfinite(v.get<double>())) throw std::runtime_error("Invalid parameter value");
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
    Enqueue([this, slot, path] { LoadNow(slot, path, false); });
  }
  Json Document(const std::vector<double>& parameters) const {
    auto view = View(); Json slots = Json::array();
    for (auto& s : view.slots) slots.push_back({{"name", s.name}, {"path", s.path}});
    return {{"version", 1}, {"parameters", parameters}, {"slots", slots}};
  }
  void Restore(Json doc) {
    ValidateDocument(doc);
    { std::lock_guard<std::mutex> lock(mutex_);
      for (int i = 0; i < kSlots; ++i) { view_.slots[i].name = doc["slots"][i]["name"].get<std::string>(); view_.slots[i].path = doc["slots"][i]["path"].get<std::string>(); }
    }
    Enqueue([this, doc] { RestoreNow(doc); });
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
  void SaveKit(std::string path, std::vector<double> parameters) {
    Enqueue([this, path, parameters] {
      namespace fs = std::filesystem;
      fs::path target(path), temporary; bool ownTemporary = false;
      try {
        if (target.extension() != ".padkit") target += ".padkit";
        if (fs::exists(target)) throw std::runtime_error("Choose a new kit name; existing kits are never overwritten");
        temporary = target.string() + ".partial";
        if (!fs::create_directory(temporary)) throw std::runtime_error("Kit staging folder already exists; choose another name");
        ownTemporary = true;
        fs::create_directory(temporary / "Samples");
        auto doc = Document(parameters);
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
      for (size_t i = 0; i < sample->Frames(); ++i) {
        size_t bin = std::min<size_t>(127, i * 128 / sample->Frames());
        for (int c = 0; c < sample->channels; ++c) update.waveform[bin] = std::max(update.waveform[bin], std::abs(sample->data[i * sample->channels + c]));
      }
      // Retain in library before publishing; vector allocation cannot strand an audio reference.
      auto* raw = sample.get(); raw->Retain(); assets_.push_back(std::move(sample));
      if (!publisher_(slot, raw)) { raw->Release(); throw std::runtime_error("Audio update queue full; start playback and retry"); }
      if (current_[slot]) current_[slot]->Release();
      current_[slot] = raw;
      { std::lock_guard<std::mutex> lock(mutex_); update.name = view_.slots[slot].name; view_.slots[slot] = update; }
    } catch (const std::exception& e) {
      std::lock_guard<std::mutex> lock(mutex_);
      view_.slots[slot].status = e.what();
      if (restoring) view_.slots[slot].path = path;
    }
  }
  void RestoreNow(const Json& doc) {
    for (int i = 0; i < kSlots; ++i) {
      if (!publisher_(i, nullptr)) { Message("Audio update queue full; start playback and reopen kit"); return; }
      if (current_[i]) { current_[i]->Release(); current_[i] = nullptr; }
      { std::lock_guard<std::mutex> lock(mutex_); view_.slots[i] = SlotView{}; view_.slots[i].name = doc["slots"][i]["name"].get<std::string>(); view_.slots[i].path = doc["slots"][i]["path"].get<std::string>(); }
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
