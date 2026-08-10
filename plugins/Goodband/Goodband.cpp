#include "Goodband.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

#include "IControls.h"
#include "GoodbandCharacterPresets.h"
#include "GoodbandSceneControl.h"
#include "../shared/WaveFactoryUI.h"
#endif

#if IPLUG_EDITOR
namespace {
constexpr const char* kGoodbandTitleFontId = "GoodbandTitle";
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
    graphics->LoadFont(kGoodbandTitleFontId, GOODBAND_TITLE_FONT_FN);
    const auto background = graphics->LoadBitmap(TEMPLE_OF_MASTERY_BG_FN);
    graphics->AttachControl(new GoodbandSceneControl(bounds, background, kAmount, kCharacter, kMix, kOutputTrim));

    graphics->AttachControl(new ITextControl(IRECT(35.0F, 20.0F, 355.0F, 80.0F), "GOODBAND",
                                             IText(46.0F, IColor(255, 239, 226, 196),
                                                   kGoodbandTitleFontId, EAlign::Near)));
    graphics->AttachControl(new ITextControl(IRECT(38.0F, 78.0F, 355.0F, 103.0F),
                                             "WAVE FACTORY ESSENTIALS  /  MASTERING ENERGY",
                                             IText(10.0F, IColor(255, 170, 153, 116), DEFAULT_FONT, EAlign::Near)));

    const auto controlStyle = wfe::ui::MakeCinematicControlStyle(IColor(255, 122, 203, 159));
    graphics->AttachControl(new IVKnobControl(IRECT(35.0F, 130.0F, 137.0F, 250.0F), kAmount,
                                              "AMOUNT", controlStyle));
    graphics->AttachControl(new IVKnobControl(IRECT(151.0F, 130.0F, 253.0F, 250.0F), kMix,
                                              "MIX", controlStyle));
    graphics->AttachControl(new IVKnobControl(IRECT(267.0F, 130.0F, 369.0F, 250.0F), kOutputTrim,
                                              "OUTPUT", controlStyle));

    const auto characterStyle = controlStyle.WithDrawFrame(true)
                                    .WithRoundness(0.04F)
                                    .WithWidgetFrac(0.70F)
                                    .WithLabelText(IText(11.0F, IColor(255, 199, 187, 159)));
    auto* characterControl = new IVTabSwitchControl(
        IRECT(35.0F, 302.0F, 369.0F, 376.0F), kCharacter, {"CLEAN", "WARM", "PUNCH", "WIDE"},
        "CHARACTER", characterStyle, EVShape::Rectangle, EDirection::Horizontal);
    characterControl->SetActionFunction([this, graphics](IControl* caller) {
      const auto characterIndex = std::clamp(static_cast<int>(std::lround(caller->GetValue() * 3.0)), 0, 3);
      ApplyCharacterPreset(characterIndex, graphics);
    });
    graphics->AttachControl(characterControl);
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
