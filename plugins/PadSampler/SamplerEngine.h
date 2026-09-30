#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>
#include <utility>
#include "ToneCurve.h"
#include "FxDSP.h"

namespace padsampler {
constexpr int kSlots = 6, kVoices = 64;
constexpr size_t kMemoryLimit = 256u * 1024u * 1024u;
static_assert(std::atomic<uint64_t>::is_always_lock_free, "Clip publication must be lock-free on the audio thread");
struct Sample {
  std::vector<float> data;
  double rate = 44100.;
  int channels = 1;
  std::atomic<int> references{0}; // Only the library reclaims zero-reference assets.
  // One atomic word is a complete playback window. An end of zero means all frames.
  std::atomic<uint64_t> trim{0};
  size_t Frames() const { return data.size() / channels; }
  void SetTrim(uint32_t start, uint32_t end) { trim.store((uint64_t(start) << 32) | end, std::memory_order_release); }
  std::pair<size_t,size_t> Trim() const {
    const auto value = trim.load(std::memory_order_acquire);
    const size_t start = size_t(value >> 32), storedEnd = size_t(uint32_t(value));
    const size_t end = storedEnd ? storedEnd : Frames();
    return start < end && end <= Frames() && end - start >= 2 ? std::pair<size_t,size_t>{start,end} : std::pair<size_t,size_t>{0,Frames()};
  }
  void Retain() { references.fetch_add(1, std::memory_order_relaxed); }
  void Release() { references.fetch_sub(1, std::memory_order_release); }
};
struct Settings {
  double level = 0., pan = 0., velocityAmount = 1.;
  double softHz = 1500., hardHz = 18000., curve = 1.;
  ToneShape shape;
  bool bypass = false;
  int note = 36, channel = 0; // channel 0 = omni, otherwise 1..16
};
inline double Cutoff(const Settings& s, int velocity, double rate) {
  const double v = s.shape.Evaluate(velocity / 127., s.curve);
  const double low = std::clamp(s.softHz, 20., rate * .45);
  const double high = std::clamp(std::max(s.softHz, s.hardHz), 20., rate * .45);
  return std::exp(std::log(low) + v * (std::log(high) - std::log(low)));
}
// Single background producer, single audio consumer. No allocation or mutex in either endpoint.
template<class T, size_t N> class Queue {
 public:
  bool Push(const T& v) {
    auto w = write_.load(std::memory_order_relaxed), next = (w + 1) % N;
    if (next == read_.load(std::memory_order_acquire)) return false;
    values_[w] = v; write_.store(next, std::memory_order_release); return true;
  }
  bool Pop(T& v) {
    auto r = read_.load(std::memory_order_relaxed);
    if (r == write_.load(std::memory_order_acquire)) return false;
    v = values_[r]; read_.store((r + 1) % N, std::memory_order_release); return true;
  }
 private:
  std::array<T, N> values_{};
  std::atomic<size_t> read_{0}, write_{0};
};
// Filter is injected: the instrument and tests use iPlug2's existing stereo SVF adapter.
template<class Filter> class Engine {
 public:
  struct Replacement { int slot; Sample* sample; };
  struct Event { int offset, note, channel, velocity; };
  struct Voice {
    Sample* sample = nullptr;
    double position = 0., step = 1., gainL = 1., gainR = 1.;
    double lastL = 0., lastR = 0., tailL = 0., tailR = 0.;
    int age = 0, release = 0, tail = 0;
    uint64_t order = 0;
    int slot = 0, tailSlot = 0;
    size_t endFrame = 0;
    bool bypass = false;
    Filter filter;
  };
  std::array<Settings, kSlots> settings{}; // Written by adapter only on audio thread.
  FxChainExchange::Bank fxChains{};
  std::array<FxSettings,kSlots> fxSettings{};
  std::array<std::atomic<int>, kSlots> hits{}, auditions{};
  std::atomic<bool> stop{false};
  std::atomic<int> learn{-1}, learned{-1}, lastMidi{-1};
  std::atomic<unsigned> droppedEvents{0};
  double master = .5;
  Engine():fx_(new FxDSP[kSlots]){}
  ~Engine() { Shutdown(); }
  void Shutdown() {
    for (auto& v : voices_) if (v.sample) { v.sample->Release(); v.sample = nullptr; }
    for (auto*& s : samples_) if (s) { s->Release(); s = nullptr; }
    Replacement r; while (replacements_.Pop(r)) if (r.sample) r.sample->Release();
  }
  bool Publish(int slot, Sample* sample) {
    if (sample) sample->Retain();
    if (replacements_.Push({slot, sample})) return true;
    if (sample) sample->Release();
    return false;
  }
  void Reset(double rate) {
    rate_ = std::isfinite(rate) && rate >= 8000. ? rate : 44100.;
    for(int i=0;i<kSlots;++i)fx_[i].SetRate(rate_);
    for (auto& v : voices_) {
      if (v.sample) v.sample->Release();
      v = Voice{};
    }
    eventCount_ = 0;
  }
  void Midi(int offset, int note, int channel, int velocity) {
    if (velocity <= 0 || note < 0 || note > 127 || channel < 1 || channel > 16) return;
    const int packed = note | (channel << 8) | (velocity << 16);
    lastMidi.store(packed);
    int slot = learn.exchange(-1);
    if (slot >= 0 && slot < kSlots) learned.store(packed | (slot << 24));
    if (eventCount_ == events_.size()) { droppedEvents.fetch_add(1); return; }
    Event e{std::max(0, offset), note, channel, std::min(127, velocity)};
    size_t i = eventCount_++;
    while (i > 0 && events_[i - 1].offset > e.offset) { events_[i] = events_[i - 1]; --i; }
    events_[i] = e;
  }
  void Trigger(int slot, int velocity) {
    auto* sample = samples_[slot];
    if (!sample || sample->Frames() < 2) return;
    const auto [start, end] = sample->Trim();
    auto it = std::find_if(voices_.begin(), voices_.end(), [](const auto& v) { return !v.sample && !v.tail; });
    if (it == voices_.end()) it = std::min_element(voices_.begin(), voices_.end(), [](const auto& a, const auto& b) { return a.order < b.order; });
    auto& v = *it;
    double tailL = v.lastL, tailR = v.lastR;int tailSlot=v.slot;
    if (v.sample) v.sample->Release();
    v = Voice{};
    v.tailL = tailL; v.tailR = tailR; v.tail = fadeFrames_;
    v.slot=slot;v.tailSlot=tailSlot;
    v.sample = sample; sample->Retain();
    v.position = double(start); v.endFrame = end;
    v.step = sample->rate / rate_; v.order = ++order_;
    const auto& s = settings[slot];
    double gain = std::pow(10., s.level / 20.) * ((1. - s.velocityAmount) + s.velocityAmount * velocity / 127.);
    // Balance pan preserves stereo; center leaves both channels unchanged.
    v.gainL = gain * std::min(1., 1. - s.pan);
    v.gainR = gain * std::min(1., 1. + s.pan);
    v.bypass = s.bypass;
    v.filter.Prepare(rate_, Cutoff(s, velocity, rate_));
    hits[slot].store(velocity);
  }
  template<class T> void Process(T* left, T* right, int frames) {
    Replacement r;
    for (int updates = 0; updates < 63 && replacements_.Pop(r); ++updates) {
      if (samples_[r.slot]) samples_[r.slot]->Release();
      samples_[r.slot] = r.sample; // Queue reference becomes slot reference.
    }
    fadeFrames_ = std::max(8, int(rate_ * .001));
    for(int i=0;i<kSlots;++i)fx_[i].SetChain(fxChains[i]);
    if (stop.exchange(false)) {
      eventCount_ = 0;
      for (auto& audition : auditions) audition.store(0);
      for (auto& v : voices_) if (v.sample) v.release = fadeFrames_;
      for(int i=0;i<kSlots;++i)fx_[i].Stop();
    }
    for (int i = 0; i < kSlots; ++i) if (int velocity = auditions[i].exchange(0)) Trigger(i, velocity);
    size_t event = 0;
    for (int f = 0; f < frames; ++f) {
      while (event < eventCount_ && events_[event].offset <= f) {
        auto& e = events_[event++];
        for (int i = 0; i < kSlots; ++i) if (settings[i].note == e.note && (!settings[i].channel || settings[i].channel == e.channel)) Trigger(i, e.velocity);
      }
      double l = 0., rr = 0.;
      std::array<double,kSlots> busL{},busR{};
      for (auto& v : voices_) {
        double vl = 0., vr = 0.;
        if (v.sample) {
          const auto& s = *v.sample;
          size_t a = size_t(v.position), b = std::min(a + 1, v.endFrame - 1);
          double frac = v.position - a;
          auto read = [&](int c) { return s.data[a * s.channels + c] * (1. - frac) + s.data[b * s.channels + c] * frac; };
          vl = read(0); vr = read(s.channels - 1);
          if (!v.bypass) v.filter.Process(vl, vr);
          double envelope = std::min(1., double(++v.age) / fadeFrames_);
          envelope *= std::min(1., (v.endFrame - v.position) / (v.step * fadeFrames_));
          bool releasing = v.release > 0;
          if (v.release) envelope *= double(v.release--) / fadeFrames_;
          vl *= envelope * v.gainL; vr *= envelope * v.gainR;
          v.position += v.step;
          if (v.position >= v.endFrame || (releasing && v.release == 0)) { v.sample->Release(); v.sample = nullptr; }
        }
        busL[v.slot]+=vl;busR[v.slot]+=vr;
        if (v.tail) { double tl=v.tailL*v.tail/fadeFrames_,tr=v.tailR*v.tail/fadeFrames_;busL[v.tailSlot]+=tl;busR[v.tailSlot]+=tr;vl+=tl;vr+=tr;--v.tail; }
        v.lastL = vl; v.lastR = vr; l += vl; rr += vr;
      }
      bool anyFX=false;
      for(int i=0;i<kSlots;++i)if(fx_[i].Active()){
        anyFX=true;
        double beforeL=busL[i],beforeR=busR[i];fx_[i].Process(busL[i],busR[i],fxSettings[i]);
        l+=busL[i]-beforeL;rr+=busR[i]-beforeR;
      }
      left[f] = T(l * master); right[f] = T(rr * master);
    }
    // Retain offsets beyond this block rather than playing them early or dropping them.
    size_t remaining = 0;
    for (; event < eventCount_; ++event) { auto e = events_[event]; e.offset -= frames; events_[remaining++] = e; }
    eventCount_ = remaining;
  }
  int ActiveVoices() const { return int(std::count_if(voices_.begin(), voices_.end(), [](const auto& v) { return v.sample != nullptr; })); }
 private:
  Queue<Replacement, 64> replacements_;
  std::array<Sample*, kSlots> samples_{};
  std::array<Voice, kVoices> voices_{};
  std::unique_ptr<FxDSP[]> fx_;
  std::array<Event, 512> events_{};
  size_t eventCount_ = 0;
  uint64_t order_ = 0;
  double rate_ = 44100.;
  int fadeFrames_ = 44;
};
}
