#include "Goodband.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#include "GoodbandSceneControl.h"
#include "../shared/WaveFactoryUI.h"
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
    graphics->LoadFont(DEFAULT_FONT, "Arial", ETextStyle::Normal);
    const auto background = graphics->LoadBitmap(ARCADE_DOJO_BG_FN);
    graphics->AttachControl(new GoodbandSceneControl(bounds, background, kAmount, kCharacter, kMix, kOutputTrim));

    graphics->AttachControl(new ITextControl(IRECT(38.0F, 34.0F, 350.0F, 78.0F), "GOODBAND",
                                             IText(32.0F, IColor(255, 244, 239, 222), "Arial", EAlign::Near)));
    graphics->AttachControl(new ITextControl(IRECT(40.0F, 75.0F, 350.0F, 104.0F),
                                             "MULTIBAND ENERGY · SENSEI CONTROL",
                                             IText(13.0F, IColor(255, 151, 202, 185), "Arial", EAlign::Near)));

    const auto controlStyle = wfe::ui::MakeDarkControlStyle(IColor(255, 106, 224, 176));
    graphics->AttachControl(new IVKnobControl(IRECT(42.0F, 118.0F, 166.0F, 242.0F), kAmount, "Amount", controlStyle));
    graphics->AttachControl(new IVKnobControl(IRECT(194.0F, 118.0F, 318.0F, 242.0F), kMix, "Mix", controlStyle));
    graphics->AttachControl(new IVMenuButtonControl(IRECT(42.0F, 292.0F, 205.0F, 354.0F), kCharacter,
                                                    "Character", controlStyle));
    graphics->AttachControl(new IVKnobControl(IRECT(224.0F, 258.0F, 344.0F, 378.0F), kOutputTrim,
                                              "Output", controlStyle));
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
