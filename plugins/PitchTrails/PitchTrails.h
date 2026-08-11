#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "wfe/dsp/pitch_trails_processor.h"

enum EParams {
  kDelay = 0,
  kPitch,
  kFeedback,
  kDiffusion,
  kMix,
  kTempoSync,
  kNoteDivision,
  kFreeze,
  kFeedbackPath,
  kNumParams,
};

using namespace iplug;
using namespace igraphics;

class PitchTrails final : public Plugin {
 public:
  explicit PitchTrails(const InstanceInfo& info);

  void OnReset() override;

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

 private:
  wfe::dsp::PitchTrailsProcessor processor_{};
};
