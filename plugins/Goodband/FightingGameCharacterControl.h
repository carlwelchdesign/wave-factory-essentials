#pragma once

#include <algorithm>
#include <cmath>

#include "IControls.h"

class FightingGameCharacterControl final : public IVTabSwitchControl {
public:
  FightingGameCharacterControl(const IRECT& bounds, int paramIndex, const std::vector<const char*>& options,
                               const IVStyle& style)
      : IVTabSwitchControl(bounds, paramIndex, options, "", style, EVShape::Rectangle,
                           EDirection::Horizontal) {}

  void Draw(IGraphics& graphics) override { DrawWidget(graphics); }

  void DrawWidget(IGraphics& graphics) override {
    const auto selected = GetSelectedIdx();

    for (int index = 0; index < mNumStates; ++index) {
      const auto buttonBounds = mButtons.Get()[index].GetPadded(-2.0F, -1.0F, -2.0F, -1.0F);
      const auto isSelected = index == selected;
      const auto isMouseOver = index == mMouseOverButton;
      DrawPlate(graphics, buttonBounds, isSelected, isMouseOver, index);

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
          auto* selector = static_cast<FightingGameCharacterControl*>(control);
          const auto progress = std::clamp(selector->GetAnimationProgress(), 0.0, 1.0);
          selector->energyStrike_ = std::sin(progress * kPi);
          selector->SetDirty(false);
          if (progress >= 1.0) {
            selector->OnEndAnimation();
          }
        },
        kEnergyStrikeDurationMs);
  }

  static void BuildChamferedPlate(IGraphics& graphics, const IRECT& bounds, float chamfer) {
    graphics.PathClear();
    graphics.PathMoveTo(bounds.L + chamfer, bounds.T);
    graphics.PathLineTo(bounds.R - chamfer, bounds.T);
    graphics.PathLineTo(bounds.R, bounds.T + chamfer);
    graphics.PathLineTo(bounds.R, bounds.B - chamfer);
    graphics.PathLineTo(bounds.R - chamfer, bounds.B);
    graphics.PathLineTo(bounds.L + chamfer, bounds.B);
    graphics.PathLineTo(bounds.L, bounds.B - chamfer);
    graphics.PathLineTo(bounds.L, bounds.T + chamfer);
    graphics.PathClose();
  }

  void DrawPlate(IGraphics& graphics, const IRECT& bounds, bool selected, bool mouseOver, int index) const {
    const auto strike = selected ? static_cast<float>(energyStrike_) : 0.0F;

    if (selected) {
      IBlend auraBlend(EBlend::Add, 0.12F + strike * 0.20F);
      BuildChamferedPlate(graphics, bounds.GetPadded(6.0F + strike * 3.0F), 13.0F);
      graphics.PathFill(IColor(255, 111, 226, 183), &auraBlend);
    }

    BuildChamferedPlate(graphics, bounds.GetTranslated(1.5F, 2.0F), 9.0F);
    graphics.PathFill(IColor(190, 0, 0, 0));

    BuildChamferedPlate(graphics, bounds, 9.0F);
    const auto plate = selected
                           ? IPattern::CreateLinearGradient(
                                 bounds, EDirection::Vertical,
                                 {{IColor(255, 115, 83, 31), 0.0F}, {IColor(255, 56, 40, 20), 0.48F},
                                  {IColor(255, 128, 91, 32), 1.0F}})
                           : IPattern::CreateLinearGradient(
                                 bounds, EDirection::Vertical,
                                 {{IColor(245, 35, 34, 31), 0.0F}, {IColor(245, 12, 14, 15), 0.52F},
                                  {IColor(245, 27, 27, 25), 1.0F}});
    graphics.PathFill(plate);

    BuildChamferedPlate(graphics, bounds, 9.0F);
    graphics.PathStroke(selected ? IColor(255, 234, 183, 85)
                                 : (mouseOver ? IColor(255, 164, 138, 82) : IColor(255, 91, 79, 56)),
                        selected ? 2.0F : 1.0F);

    const auto inset = bounds.GetPadded(-4.0F);
    BuildChamferedPlate(graphics, inset, 6.0F);
    graphics.PathStroke(selected ? IColor(220, 255, 221, 143) : IColor(150, 52, 50, 43), 0.8F);

    if (selected) {
      const auto centerX = bounds.MW();
      const auto particleY = bounds.T - 3.0F;
      for (int particle = 0; particle < 7; ++particle) {
        const auto offset = static_cast<float>(particle - 3);
        const auto spread = 8.0F + strike * 8.0F;
        const auto radius = particle % 2 == 0 ? 1.25F : 0.8F;
        IBlend particleBlend(EBlend::Add, 0.38F + strike * 0.48F);
        graphics.FillCircle(particle % 3 == 0 ? IColor(255, 111, 226, 183) : IColor(255, 246, 191, 90),
                            centerX + offset * spread, particleY + std::abs(offset) * 1.6F, radius + strike,
                            &particleBlend);
      }

      const auto sealBounds = IRECT(bounds.R - 18.0F, bounds.T + 6.0F, bounds.R - 8.0F, bounds.T + 16.0F);
      graphics.FillRect(IColor(230, 139, 42, 30), sealBounds);
      graphics.DrawRect(IColor(255, 232, 151, 89), sealBounds, nullptr, 0.8F);
      graphics.DrawLine(IColor(230, 249, 190, 117), sealBounds.L + 2.0F, sealBounds.MH(), sealBounds.R - 2.0F,
                        sealBounds.MH(), nullptr, 0.8F);
    }

    if (index > 0) {
      graphics.FillTriangle(IColor(255, 121, 93, 47), bounds.L - 2.0F, bounds.MH(), bounds.L + 2.5F,
                            bounds.MH() - 4.0F, bounds.L + 2.5F, bounds.MH() + 4.0F);
    }
  }

  double energyStrike_ = 0.0;
};
