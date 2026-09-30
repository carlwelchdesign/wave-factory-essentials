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
  const padsampler::LibraryView& View() const { return view_; }
  int Selected() const { return selected_; }
  float Hit(int slot) const { return flashes_[slot]; }
 private:
  std::vector<double> Parameters() const;
  void ApplyParameters(const padsampler::Json& doc, bool notifyHost);
  void ChooseSample();
  void ChooseKit(bool save);
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
