#pragma once

#include <array>
#include <atomic>

#include "IPlug_include_in_plug_hdr.h"
#include "wfe/dsp/goodband_processor.h"

enum EParams {
  kAmount = 0,
  kCharacter,
  kMix,
  kOutputTrim,
  kAutoGainMatch,
  kNumParams,
};

using namespace iplug;
using namespace igraphics;

class Goodband final : public Plugin {
 public:
  explicit Goodband(const InstanceInfo& info);

  void OnReset() override;

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

 private:
#if IPLUG_EDITOR
  void ApplyCharacterPreset(int characterIndex, IGraphics* graphics);
  void ApplyFormPreset(int formIndex, IGraphics* graphics);
#endif

  wfe::dsp::GoodbandProcessor processor_{};
  std::array<std::atomic<float>, 3> gainReductionDb_{};
  std::atomic<float> autoGainCompensationDb_{0.0F};
};
