#include "Goodband.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

#include "IControls.h"
#include "GoodbandBackdropControl.h"
#include "GoodbandCharacterPresets.h"
#include "GoodbandHelpControl.h"
#include "GoodbandSceneControl.h"
#include "FightingGameCharacterControl.h"
#include "IllustratedBitmapDrawing.h"
#include "IllustratedShurikenKnobControl.h"
#include "../shared/WaveFactoryUI.h"

namespace {
constexpr float kFrameOverscanScale = 1.048F;
}
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
    const auto background = graphics->LoadBitmap(TEMPLE_OF_MASTERY_BG_FN);
    const auto gestureBackground = graphics->LoadBitmap(TEMPLE_OF_MASTERY_GESTURE_BG_FN);
    const auto wordmark = graphics->LoadBitmap(THREEFOLD_PALM_WORDMARK_FN);
    const auto frame = graphics->LoadBitmap(THREEFOLD_PALM_FRAME_FN);
    const auto unselectedPlate = graphics->LoadBitmap(THREEFOLD_PALM_UNSELECTED_PLATE_FN);
    const auto selectedPlate = graphics->LoadBitmap(THREEFOLD_PALM_SELECTED_PLATE_FN);
    const auto knobArena = graphics->LoadBitmap(THREEFOLD_PALM_KNOB_ARENA_FN);
    const auto shuriken = graphics->LoadBitmap(THREEFOLD_PALM_SHURIKEN_FN);
    const auto vfxAtlas = graphics->LoadBitmap(THREEFOLD_PALM_CHI_VFX_ATLAS_FN);
    graphics->AttachControl(new GoodbandBackdropControl(bounds, background, gestureBackground, vfxAtlas));
    graphics->AttachControl(new GoodbandSceneControl(bounds, kAmount, kCharacter, kMix, kOutputTrim, vfxAtlas));

    graphics->AttachControl(
        new threefold::AspectFitBitmapControl(IRECT(27.0F, 12.0F, 410.0F, 91.0F), wordmark));
    graphics->AttachControl(new ITextControl(IRECT(39.0F, 91.0F, 394.0F, 111.0F),
                                             "WAVE FACTORY ESSENTIALS  /  MASTERING ENERGY",
                                             IText(9.5F, IColor(255, 183, 158, 108), DEFAULT_FONT, EAlign::Near)));

    const auto controlStyle = wfe::ui::MakeCinematicControlStyle(IColor(255, 111, 226, 183))
                                  .WithLabelText(IText(12.0F, IColor(255, 232, 215, 177), DEFAULT_FONT,
                                                           EAlign::Center, EVAlign::Middle))
                                  .WithValueText(IText(15.0F, IColor(255, 238, 190, 99), DEFAULT_FONT,
                                                           EAlign::Center, EVAlign::Bottom))
                                  .WithLabelOrientation(EOrientation::North)
                                  .WithWidgetFrac(0.88F);
    graphics->AttachControl(
        new IllustratedShurikenKnobControl(IRECT(28.0F, 204.0F, 148.0F, 398.0F), kAmount, "AMOUNT",
                                           controlStyle, knobArena, shuriken));
    graphics->AttachControl(
        new IllustratedShurikenKnobControl(IRECT(145.0F, 204.0F, 265.0F, 398.0F), kMix, "MIX", controlStyle,
                                           knobArena, shuriken));
    graphics->AttachControl(
        new IllustratedShurikenKnobControl(IRECT(262.0F, 204.0F, 382.0F, 398.0F), kOutputTrim, "OUTPUT",
                                           controlStyle, knobArena, shuriken));

    const auto characterStyle = controlStyle.WithShowLabel(false)
                                    .WithShowValue(false)
                                    .WithWidgetFrac(1.0F);
    auto* characterControl = new IllustratedCharacterControl(
        IRECT(29.0F, 125.0F, 407.0F, 174.0F), kCharacter, {"CLEAN", "WARM", "PUNCH", "WIDE"},
        characterStyle, unselectedPlate, selectedPlate);
    characterControl->SetActionFunction([this, graphics](IControl* caller) {
      const auto characterIndex = std::clamp(static_cast<int>(std::lround(caller->GetValue() * 3.0)), 0, 3);
      ApplyCharacterPreset(characterIndex, graphics);
    });
    graphics->AttachControl(characterControl);

    graphics->AttachControl(
        new threefold::AspectFitBitmapControl(bounds.GetScaledAboutCentre(kFrameOverscanScale), frame));
    graphics->AttachControl(new GoodbandHelpControl(bounds));
  };
#endif
}

#if IPLUG_EDITOR
void Goodband::ApplyCharacterPreset(int characterIndex, IGraphics* graphics) {
  const auto& preset = wfe::ui::GetGoodbandCharacterPreset(characterIndex);
  const std::array<std::pair<int, double>, 3> updates{{
      {kAmount, preset.amountPercent},
      {kMix, preset.mixPercent},
      {kOutputTrim, preset.outputTrimDb},
  }};

  for (const auto& update : updates) {
    const auto paramIndex = update.first;
    const auto plainValue = update.second;
    const auto normalizedValue = GetParam(paramIndex)->ToNormalized(plainValue);
    BeginInformHostOfParamChangeFromUI(paramIndex);
    SendParameterValueFromUI(paramIndex, normalizedValue);
    EndInformHostOfParamChangeFromUI(paramIndex);

    graphics->ForControlWithParam(paramIndex, [paramIndex, normalizedValue](IControl* control) {
      const auto valueIndex = control->LinkedToParam(paramIndex);
      control->SetValueFromDelegate(normalizedValue, valueIndex);
    });
  }
}
#endif

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
