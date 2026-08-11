#include "PitchTrails.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#include "../shared/WaveFactoryUI.h"
#endif

PitchTrails::PitchTrails(const InstanceInfo& info) : iplug::Plugin(info, MakeConfig(kNumParams, 1)) {
  GetParam(kDelay)->InitDouble("Delay", 320.0, 10.0, 2000.0, 1.0, "ms");
  GetParam(kPitch)->InitDouble("Pitch", 7.0, -12.0, 12.0, 1.0, "st");
  GetParam(kFeedback)->InitDouble("Feedback", 35.0, 0.0, 92.0, 0.1, "%");
  GetParam(kDiffusion)->InitDouble("Diffusion", 35.0, 0.0, 100.0, 0.1, "%");
  GetParam(kMix)->InitDouble("Mix", 35.0, 0.0, 100.0, 0.1, "%");

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() { return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS); };
  mLayoutFunc = [&](IGraphics* graphics) {
    const auto bounds = graphics->GetBounds();
    graphics->LoadFont(DEFAULT_FONT, "Arial", ETextStyle::Normal);
    graphics->AttachPanelBackground(IColor(255, 20, 18, 29));
    graphics->AttachControl(new ITextControl(bounds.GetFromTop(72.0F), "PITCH TRAILS", IText(34.0F, COLOR_WHITE)));
    graphics->AttachControl(new ITextControl(bounds.GetFromTop(118.0F).GetFromBottom(32.0F),
                                             "MOVEMENT WITHOUT DRAWN AUTOMATION", IText(14.0F, IColor(255, 194, 165, 230))));
    const auto controls = bounds.GetReducedFromTop(135.0F).GetPadded(-28.0F);
    const auto controlStyle = wfe::ui::MakeDarkControlStyle(IColor(255, 194, 165, 230));
    graphics->AttachControl(new IVKnobControl(controls.GetGridCell(0, 0, 1, 5).GetCentredInside(112.0F), kDelay, "Delay", controlStyle));
    graphics->AttachControl(new IVKnobControl(controls.GetGridCell(0, 1, 1, 5).GetCentredInside(112.0F), kPitch, "Pitch", controlStyle));
    graphics->AttachControl(new IVKnobControl(controls.GetGridCell(0, 2, 1, 5).GetCentredInside(112.0F), kFeedback, "Feedback", controlStyle));
    graphics->AttachControl(new IVKnobControl(controls.GetGridCell(0, 3, 1, 5).GetCentredInside(112.0F), kDiffusion, "Diffusion", controlStyle));
    graphics->AttachControl(new IVKnobControl(controls.GetGridCell(0, 4, 1, 5).GetCentredInside(112.0F), kMix, "Mix", controlStyle));
  };
#endif
}

void PitchTrails::OnReset() { processor_.Prepare(GetSampleRate()); }

#if IPLUG_DSP
void PitchTrails::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
  wfe::dsp::PitchTrailsParameters parameters;
  parameters.delayMs = static_cast<float>(GetParam(kDelay)->Value());
  parameters.semitones = static_cast<float>(GetParam(kPitch)->Value());
  parameters.feedback = static_cast<float>(GetParam(kFeedback)->Value() / 100.0);
  parameters.diffusion = static_cast<float>(GetParam(kDiffusion)->Value() / 100.0);
  parameters.mix = static_cast<float>(GetParam(kMix)->Value() / 100.0);
  processor_.SetParameters(parameters);

  const auto channels = NOutChansConnected();
  for (int frame = 0; frame < nFrames; ++frame) {
    const auto left = static_cast<float>(inputs[0][frame]);
    const auto right = static_cast<float>(channels > 1 ? inputs[1][frame] : inputs[0][frame]);
    const auto processed = processor_.ProcessFrame(left, right);
    outputs[0][frame] = static_cast<sample>(processed[0]);
    if (channels > 1) {
      outputs[1][frame] = static_cast<sample>(processed[1]);
    }
  }
}
#endif
