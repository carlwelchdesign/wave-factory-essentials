#pragma once
#include "IPlug_include_in_plug_hdr.h"
#include "IPlugFilter.h"
#include "SamplerEngine.h"
#include "SampleLibrary.h"
namespace iplug { namespace igraphics { class ITextControl; } }
// Product 0.1 parameter IDs are fixed. Append; never reorder after distribution.
enum SlotParam { Level, Pan, VelocityAmount, SoftHz, HardHz, ToneCurve, ToneBypass, Note, Channel };
constexpr int kAuditionVelocity = padsampler::kSlots * padsampler::kSlotParams;
constexpr int kMaster = kAuditionVelocity + 1;
class PadSampler final : public iplug::Plugin {
 public:
  explicit PadSampler(const iplug::InstanceInfo& info);
  ~PadSampler() override;
  void OnReset() override;
  void ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int frames) override;
  void ProcessMidiMsg(const iplug::IMidiMsg& msg) override;
  void OnIdle() override;
  bool SerializeState(iplug::IByteChunk& chunk) const override;
  int UnserializeState(const iplug::IByteChunk& chunk, int position) override;
  void Select(int slot);
  void Audition(int slot);
  void Load(int slot, const std::string& path);
  padsampler::ToneShape Shape() const { return curves_.Snapshot()[selected_]; }
  double Exponent() const { return GetParam(selected_ * padsampler::kSlotParams + ToneCurve)->Value(); }
  double ToneHz(double velocity) const;
  void SetShape(const padsampler::ToneShape& shape);
  void CommitShape(const padsampler::ToneShape& before);
  void UndoCurve(bool redo);
  void CurvePreset(int preset);
  void ToggleClipMode();
  bool ClipMode() const { return clipMode_; }
  padsampler::ClipTrim Clip() const { return clipPreview_ ? *clipPreview_ : view_.slots[selected_].clip; }
  void PreviewClip(padsampler::ClipTrim clip) { clipPreview_ = clip; if (GetUI()) GetUI()->SetAllControlsDirty(); }
  void CancelClipPreview() { clipPreview_.reset(); if (GetUI()) GetUI()->SetAllControlsDirty(); }
  bool CommitClip(padsampler::ClipTrim before);
  void UndoClip(bool redo);
  void ResetClip();
  unsigned ClipEpoch() const { return clipEpoch_; }
  padsampler::ClipHistory& ClipEdits() { return clipHistories_[selected_]; }
  void MarkStateChanged();
  void Focus(iplug::igraphics::IControl* c) { focus_ = c; }
  iplug::igraphics::IControl* Focused() const { return focus_; }
  void Message(const std::string& s) { uiMessage_ = s; messageTicks_ = 120; }
  bool ReducedMotion() const { return reducedMotion_; }
  int LastHit(int slot) const { return lastHits_[slot]; }
  padsampler::CurveHistory& History() { return histories_[selected_]; }
  void ToggleLearn();
  void StopAll();
  void Settings();
  void BuildUI(iplug::igraphics::IGraphics* g);
  unsigned CurveEpoch() const { return curveEpoch_.load(); }
  int pointSelection = 1;
  const padsampler::LibraryView& View() const { return view_; }
  int Selected() const { return selected_; }
  float Hit(int slot) const { return flashes_[slot]; }
 private:
  std::vector<double> Parameters() const;
  void ApplyParameters(const padsampler::Json& doc, bool notifyHost);
  void ChooseSample();
  void ChooseKit(bool save);
  padsampler::CurveExchange curves_;
  padsampler::CurveExchange::Bank audioCurves_{};
  std::array<padsampler::CurveHistory, 6> histories_{};
  std::array<padsampler::ClipHistory, 6> clipHistories_{};
  std::optional<padsampler::ClipTrim> clipPreview_;
  bool clipMode_ = false;
  unsigned clipEpoch_ = 0;
  std::vector<iplug::igraphics::IControl*> toneControls_, clipControls_;
  void SyncClipMode();
  std::atomic<bool> resetHistories_{false};
  std::atomic<unsigned> curveEpoch_{0};
  std::array<int,6> lastHits_{};
  iplug::igraphics::IControl* focus_ = nullptr;
  std::vector<iplug::igraphics::IControl*> focusOrder_;
  bool reducedMotion_ = false;
  std::string uiMessage_;
  int messageTicks_ = 0;
  padsampler::Engine<padsampler::IPlugFilter> engine_;
  padsampler::SampleLibrary library_;
  padsampler::LibraryView view_;
  std::array<float, padsampler::kSlots> flashes_{};
  int selected_ = 0;
  std::string midiText_ = "USB MIDI: hit a pad to see its note and velocity";
  std::array<iplug::igraphics::IControl*, padsampler::kSlotParams> controls_{};
  iplug::igraphics::ITextControl* monitor_ = nullptr;
  iplug::igraphics::ITextControl* status_ = nullptr;
  iplug::igraphics::ITextControl* name_ = nullptr;
  iplug::igraphics::ITextControl* mapping_ = nullptr;
  iplug::igraphics::IControl* help_ = nullptr;
};
