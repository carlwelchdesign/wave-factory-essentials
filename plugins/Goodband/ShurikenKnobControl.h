#pragma once

#include <cmath>

#include "IControls.h"

class ShurikenKnobControl final : public IVKnobControl {
public:
  ShurikenKnobControl(const IRECT& bounds, int paramIndex, const char* label, const IVStyle& style)
      : IVKnobControl(bounds, paramIndex, label, style, true) {}

  void DrawWidget(IGraphics& graphics) override {
    const auto radius = GetRadius();
    const auto centerX = mWidgetBounds.MW();
    const auto centerY = mWidgetBounds.MH();
    const auto angleDegrees = mAngle1 + static_cast<float>(GetValue()) * (mAngle2 - mAngle1);
    const auto angleRadians = (angleDegrees - 90.0F) * kPi / 180.0F;

    DrawIndicatorTrack(graphics, angleDegrees, centerX, centerY, radius);

    IBlend glowBlend(EBlend::Add, mMouseDown ? 0.30F : 0.16F);
    graphics.FillCircle(GetColor(kPR), centerX, centerY, radius * 0.78F, &glowBlend);

    BuildShurikenPath(graphics, centerX + 1.5F, centerY + 2.0F, radius * 0.76F, radius * 0.28F,
                      angleRadians);
    graphics.PathFill(IColor(165, 0, 0, 0));

    BuildShurikenPath(graphics, centerX, centerY, radius * 0.76F, radius * 0.28F, angleRadians);
    const auto metal = IPattern::CreateLinearGradient(
        mWidgetBounds, EDirection::Vertical,
        {{IColor(255, 239, 226, 191), 0.0F}, {IColor(255, 119, 108, 83), 0.52F},
         {IColor(255, 224, 190, 111), 1.0F}});
    graphics.PathFill(metal);

    BuildShurikenPath(graphics, centerX, centerY, radius * 0.76F, radius * 0.28F, angleRadians);
    graphics.PathStroke(IColor(255, 63, 57, 45), 1.1F);

    graphics.FillCircle(IColor(255, 27, 30, 29), centerX, centerY, radius * 0.15F);
    graphics.DrawCircle(IColor(255, 224, 190, 111), centerX, centerY, radius * 0.15F, nullptr, 1.4F);

    const auto pointerX = centerX + std::cos(angleRadians) * radius * 0.62F;
    const auto pointerY = centerY + std::sin(angleRadians) * radius * 0.62F;
    IBlend pointerGlow(EBlend::Add, 0.55F);
    graphics.FillCircle(GetColor(kPR), pointerX, pointerY, 2.7F, &pointerGlow);
    graphics.FillCircle(IColor(255, 243, 255, 246), pointerX, pointerY, 1.15F);
  }

private:
  static constexpr float kPi = 3.14159265358979323846F;

  static void BuildShurikenPath(IGraphics& graphics, float centerX, float centerY, float outerRadius,
                                float innerRadius, float rotation) {
    for (int point = 0; point < 8; ++point) {
      const auto angle = rotation + static_cast<float>(point) * kPi / 4.0F;
      const auto radius = point % 2 == 0 ? outerRadius : innerRadius;
      const auto x = centerX + std::cos(angle) * radius;
      const auto y = centerY + std::sin(angle) * radius;
      if (point == 0) {
        graphics.PathMoveTo(x, y);
      } else {
        graphics.PathLineTo(x, y);
      }
    }
    graphics.PathClose();
  }
};
