#include "PitchTrails.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#include "IllustratedSpiritDialControl.h"
#include "ValleySpiritBackdropControl.h"
#include "ValleySpiritBitmapControl.h"
#include "ValleySpiritHelpControl.h"
#include "ValleySpiritSceneControl.h"
#include "../shared/EngravedTabControl.h"
#include "../shared/WaveFactoryUI.h"
#include "wfe/dsp/tempo_sync.h"

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
  GetParam(kTempoSync)->InitBool("Tempo Sync", false);
  GetParam(kNoteDivision)->InitEnum(
      "Note Division", 5, {"1/16", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1 BAR"});
  GetParam(kFreeze)->InitBool("Freeze", false);
  GetParam(kFeedbackPath)->InitEnum("Feedback Path", 2, {"Reflection", "Spiral", "Cloud"});

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
        bounds, kDelay, kPitch, kFeedback, kDiffusion, kMix, kFreeze, kFeedbackPath,
        background, gestureBackground,
        vfxAtlas));
    graphics->AttachControl(new ValleySpiritSceneControl(
        bounds, kDelay, kPitch, kFeedback, kDiffusion, kMix, kFreeze, kFeedbackPath,
        vfxAtlas));

    graphics->AttachControl(new ITextControl(
        IRECT(24.0F, 27.0F, 384.0F, 86.0F), "VALLEY SPIRIT",
        IText(43.0F, IColor(255, 222, 214, 196), VALLEY_SPIRIT_FONT, EAlign::Center,
              EVAlign::Middle)));
    graphics->AttachControl(new ITextControl(
        IRECT(24.0F, 82.0F, 384.0F, 106.0F), "THE VALLEY ANSWERS WITHOUT END",
        IText(10.5F, IColor(255, 181, 153, 107), VALLEY_SPIRIT_FONT, EAlign::Center,
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

    const auto menuStyle =
        wfe::ui::MakeCinematicControlStyle(IColor(255, 151, 196, 231))
            .WithShowLabel(false)
            .WithShowValue(true)
            .WithValueText(IText(9.0F, IColor(255, 222, 214, 196), VALLEY_SPIRIT_FONT,
                                 EAlign::Center, EVAlign::Middle))
            .WithWidgetFrac(1.0F);

    graphics->AttachControl(new wfe::ui::EngravedTabControl(
        IRECT(24.0F, 109.0F, 100.0F, 132.0F), kTempoSync, {"FREE", "SYNC"},
        VALLEY_SPIRIT_FONT, IColor(255, 151, 196, 231)));
    graphics->AttachControl(new IVMenuButtonControl(
        IRECT(104.0F, 109.0F, 205.0F, 132.0F), kNoteDivision, "", menuStyle));
    graphics->AttachControl(new wfe::ui::EngravedTabControl(
        IRECT(209.0F, 109.0F, 325.0F, 132.0F), kFeedbackPath,
        {"REFLECT", "SPIRAL", "CLOUD"}, VALLEY_SPIRIT_FONT,
        IColor(255, 151, 196, 231)));
    graphics->AttachControl(new wfe::ui::EngravedTabControl(
        IRECT(329.0F, 109.0F, 408.0F, 132.0F), kFreeze, {"LIVE", "FREEZE"},
        VALLEY_SPIRIT_FONT, IColor(255, 190, 151, 92)));

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
  const auto tempoSync = GetParam(kTempoSync)->Bool();
  const auto division = static_cast<wfe::dsp::DelayDivision>(GetParam(kNoteDivision)->Int());
  int timeSigNumerator = 4;
  int timeSigDenominator = 4;
  GetTimeSig(timeSigNumerator, timeSigDenominator);
  parameters.delayMs = tempoSync
                           ? wfe::dsp::DelayMilliseconds(GetTempo(), division, timeSigNumerator,
                                                        timeSigDenominator)
                           : static_cast<float>(GetParam(kDelay)->Value());
  parameters.semitones = static_cast<float>(GetParam(kPitch)->Value());
  parameters.feedback = static_cast<float>(GetParam(kFeedback)->Value() / 100.0);
  parameters.diffusion = static_cast<float>(GetParam(kDiffusion)->Value() / 100.0);
  parameters.mix = static_cast<float>(GetParam(kMix)->Value() / 100.0);
  parameters.freeze = GetParam(kFreeze)->Bool();
  parameters.feedbackPath =
      static_cast<wfe::dsp::PitchTrailsFeedbackPath>(GetParam(kFeedbackPath)->Int());
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
