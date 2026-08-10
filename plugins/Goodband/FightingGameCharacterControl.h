#pragma once

#include <algorithm>
#include "IControls.h"
#include "IllustratedBitmapDrawing.h"

class IllustratedCharacterControl final : public IVTabSwitchControl {
public:
  IllustratedCharacterControl(const IRECT& bounds, int paramIndex, const std::vector<const char*>& options,
                              const IVStyle& style, const IBitmap& unselectedPlate,
                              const IBitmap& selectedPlate)
      : IVTabSwitchControl(bounds, paramIndex, options, "", style, EVShape::Rectangle,
                           EDirection::Horizontal),
        unselectedPlate_(unselectedPlate),
        selectedPlate_(selectedPlate) {}

  void Draw(IGraphics& graphics) override { DrawWidget(graphics); }

  void DrawWidget(IGraphics& graphics) override {
    const auto selected = GetSelectedIdx();

    for (int index = 0; index < mNumStates; ++index) {
      const auto buttonBounds = mButtons.Get()[index];
      const auto isSelected = index == selected;
      const auto isMouseOver = index == mMouseOverButton;
      const auto pulseScale = isSelected ? static_cast<float>(energyStrike_) * 2.5F : 0.0F;
      const auto plateBounds = buttonBounds.GetPadded(2.0F + pulseScale, 4.0F + pulseScale,
                                                       2.0F + pulseScale, 4.0F + pulseScale);
      IBlend plateBlend(EBlend::Default, isMouseOver || isSelected ? 1.0F : 0.82F);
      threefold::DrawThreeSlicePlate(graphics, isSelected ? selectedPlate_ : unselectedPlate_, plateBounds,
                                    &plateBlend);

      if (mTabLabels.Get(index)) {
        const auto textColor = isSelected ? IColor(255, 255, 232, 181)
                                          : (isMouseOver ? IColor(255, 235, 218, 183)
                                                         : IColor(255, 197, 184, 153));
        graphics.DrawText(IText(isSelected ? 13.0F : 12.0F, textColor, DEFAULT_FONT, EAlign::Center,
                                EVAlign::Middle),
                          mTabLabels.Get(index)->Get(), buttonBounds);
      }
    }
  }

  void OnRescale() override {
    IVTabSwitchControl::OnRescale();
    unselectedPlate_ = GetUI()->GetScaledBitmap(unselectedPlate_);
    selectedPlate_ = GetUI()->GetScaledBitmap(selectedPlate_);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override {
    const auto previous = GetSelectedIdx();
    IVTabSwitchControl::OnMouseDown(x, y, mod);
    if (GetSelectedIdx() != previous) {
      BeginEnergyStrike();
    }
  }

  void SetValueFromDelegate(double value, int valueIndex = 0) override {
    const auto previous = GetSelectedIdx();
    IVTabSwitchControl::SetValueFromDelegate(value, valueIndex);
    if (GetSelectedIdx() != previous) {
      BeginEnergyStrike();
    }
  }

private:
  static constexpr int kEnergyStrikeDurationMs = 520;
  static constexpr float kPi = 3.14159265358979323846F;

  void BeginEnergyStrike() {
    energyStrike_ = 0.0;
    SetAnimation(
        [](IControl* control) {
          auto* selector = static_cast<IllustratedCharacterControl*>(control);
          const auto progress = std::clamp(selector->GetAnimationProgress(), 0.0, 1.0);
          selector->energyStrike_ = std::sin(progress * kPi);
          selector->SetDirty(false);
          if (progress >= 1.0) {
            selector->OnEndAnimation();
          }
        },
        kEnergyStrikeDurationMs);
  }

  IBitmap unselectedPlate_;
  IBitmap selectedPlate_;
  double energyStrike_ = 0.0;
};
