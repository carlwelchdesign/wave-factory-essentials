#include "Goodband.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#endif

Goodband::Goodband(const InstanceInfo& info) : iplug::Plugin(info, MakeConfig(kNumParams, 1)) {
  GetParam(kAmount)->InitDouble("Amount", 35.0, 0.0, 100.0, 0.1, "%");
  GetParam(kCharacter)->InitEnum("Character", 0, {"Clean", "Warm", "Punch", "Wide"});
  GetParam(kMix)->InitDouble("Mix", 100.0, 0.0, 100.0, 0.1, "%");
  GetParam(kOutputTrim)->InitDouble("Output", 0.0, -12.0, 12.0, 0.1, "dB");

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() { return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS); };
  mLayoutFunc = [&](IGraphics* graphics) {
    const auto bounds = graphics->GetBounds();
    graphics->AttachPanelBackground(IColor(255, 18, 22, 25));
    graphics->AttachControl(new ITextControl(bounds.GetFromTop(72.0F), "GOODBAND", IText(34.0F, COLOR_WHITE)));
    graphics->AttachControl(new ITextControl(bounds.GetFromTop(118.0F).GetFromBottom(32.0F),
                                             "MULTIBAND ENERGY · HONEST CONTROL", IText(14.0F, IColor(255, 150, 180, 170))));
    const auto controls = bounds.GetReducedFromTop(130.0F).GetPadded(-34.0F);
    graphics->AttachControl(new IVKnobControl(controls.GetGridCell(0, 0, 1, 4).GetCentredInside(118.0F), kAmount, "Amount"));
    graphics->AttachControl(new IVMenuButtonControl(controls.GetGridCell(0, 1, 1, 4).GetCentredInside(138.0F, 54.0F), kCharacter, "Character"));
    graphics->AttachControl(new IVKnobControl(controls.GetGridCell(0, 2, 1, 4).GetCentredInside(118.0F), kMix, "Mix"));
    graphics->AttachControl(new IVKnobControl(controls.GetGridCell(0, 3, 1, 4).GetCentredInside(118.0F), kOutputTrim, "Output"));
  };
#endif
}

void Goodband::OnReset() { processor_.Prepare(GetSampleRate()); }

#if IPLUG_DSP
void Goodband::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
  wfe::dsp::GoodbandParameters parameters;
  parameters.amount = static_cast<float>(GetParam(kAmount)->Value() / 100.0);
  parameters.character = static_cast<wfe::dsp::GoodbandCharacter>(GetParam(kCharacter)->Int());
  parameters.mix = static_cast<float>(GetParam(kMix)->Value() / 100.0);
  parameters.outputTrimDb = static_cast<float>(GetParam(kOutputTrim)->Value());
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
