#include "PitchTrails.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#include "IllustratedSpiritDialControl.h"
#include "ValleySpiritBackdropControl.h"
#include "ValleySpiritBitmapControl.h"
#include "ValleySpiritHelpControl.h"
#include "ValleySpiritSceneControl.h"
#include "../shared/WaveFactoryUI.h"

namespace {
constexpr float kFrameOverscanScale = 1.048F;
}
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
    graphics->LoadFont(VALLEY_SPIRIT_FONT, VALLEY_SPIRIT_FONT_FN);

    const auto background = graphics->LoadBitmap(VALLEY_SPIRIT_BG_FN);
    const auto gestureBackground = graphics->LoadBitmap(VALLEY_SPIRIT_GESTURE_BG_FN);
    const auto frame = graphics->LoadBitmap(VALLEY_SPIRIT_FRAME_FN);
    const auto moonstoneDial = graphics->LoadBitmap(VALLEY_SPIRIT_MOONSTONE_DIAL_FN);
    const auto jadeDial = graphics->LoadBitmap(VALLEY_SPIRIT_JADE_DIAL_FN);
    const auto vfxAtlas = graphics->LoadBitmap(VALLEY_SPIRIT_VFX_ATLAS_FN);

    graphics->AttachControl(new ValleySpiritBackdropControl(
        bounds, kDelay, kPitch, kFeedback, kDiffusion, kMix, background, gestureBackground,
        vfxAtlas));
    graphics->AttachControl(new ValleySpiritSceneControl(
        bounds, kDelay, kPitch, kFeedback, kDiffusion, kMix, vfxAtlas));

    graphics->AttachControl(new ITextControl(
        IRECT(42.0F, 30.0F, 402.0F, 84.0F), "VALLEY SPIRIT",
        IText(37.0F, IColor(255, 222, 214, 196), VALLEY_SPIRIT_FONT, EAlign::Near,
              EVAlign::Middle)));
    graphics->AttachControl(new ITextControl(
        IRECT(77.0F, 82.0F, 390.0F, 104.0F), "THE VALLEY ANSWERS WITHOUT END",
        IText(9.0F, IColor(255, 181, 153, 107), VALLEY_SPIRIT_FONT, EAlign::Near,
              EVAlign::Middle)));

    const auto primaryStyle =
        wfe::ui::MakeCinematicControlStyle(IColor(255, 176, 222, 246))
            .WithLabelText(IText(12.0F, IColor(255, 226, 214, 190), VALLEY_SPIRIT_FONT,
                                 EAlign::Center, EVAlign::Middle))
            .WithValueText(IText(14.0F, IColor(255, 218, 193, 143), VALLEY_SPIRIT_FONT,
                                 EAlign::Center, EVAlign::Bottom))
            .WithLabelOrientation(EOrientation::North)
            .WithWidgetFrac(0.82F);
    const auto secondaryStyle =
        primaryStyle
            .WithLabelText(IText(10.5F, IColor(255, 218, 211, 194), VALLEY_SPIRIT_FONT,
                                 EAlign::Center, EVAlign::Middle))
            .WithValueText(IText(12.5F, IColor(255, 199, 184, 151), VALLEY_SPIRIT_FONT,
                                 EAlign::Center, EVAlign::Bottom))
            .WithWidgetFrac(0.78F);

    graphics->AttachControl(new IllustratedSpiritDialControl(
        IRECT(24.0F, 132.0F, 148.0F, 326.0F), kDelay, "ECHO TIME", primaryStyle,
        moonstoneDial, 1.10F));
    graphics->AttachControl(new IllustratedSpiritDialControl(
        IRECT(142.0F, 132.0F, 266.0F, 326.0F), kPitch, "PITCH", primaryStyle,
        moonstoneDial, 1.10F));
    graphics->AttachControl(new IllustratedSpiritDialControl(
        IRECT(260.0F, 132.0F, 384.0F, 326.0F), kFeedback, "FEEDBACK", primaryStyle,
        moonstoneDial, 1.10F));
    graphics->AttachControl(new IllustratedSpiritDialControl(
        IRECT(91.0F, 307.0F, 197.0F, 449.0F), kDiffusion, "DIFFUSION", secondaryStyle,
        jadeDial, 1.04F));
    graphics->AttachControl(new IllustratedSpiritDialControl(
        IRECT(215.0F, 307.0F, 321.0F, 449.0F), kMix, "MIX", secondaryStyle, jadeDial,
        1.04F));

    graphics->AttachControl(new ValleySpiritBitmapControl(
        bounds.GetScaledAboutCentre(kFrameOverscanScale), frame));
    graphics->AttachControl(new ValleySpiritHelpControl(bounds));
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
