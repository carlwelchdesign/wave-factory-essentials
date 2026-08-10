#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "wfe/dsp/goodband_processor.h"

enum EParams {
  kAmount = 0,
  kCharacter,
  kMix,
  kOutputTrim,
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
#endif

  wfe::dsp::GoodbandProcessor processor_{};
};
