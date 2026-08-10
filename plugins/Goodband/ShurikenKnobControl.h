#pragma once

#include <algorithm>
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

    DrawArenaRing(graphics, centerX, centerY, radius, angleRadians);
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

    graphics.FillCircle(IColor(255, 18, 20, 20), centerX, centerY, radius * 0.20F);
    graphics.DrawCircle(IColor(255, 224, 190, 111), centerX, centerY, radius * 0.20F, nullptr, 1.6F);
    graphics.DrawCircle(IColor(255, 91, 79, 55), centerX, centerY, radius * 0.12F, nullptr, 0.9F);

    const auto needleLength = radius * 0.60F;
    const auto needleWidth = std::max(2.6F, radius * 0.065F);
    const auto perpendicularX = -std::sin(angleRadians) * needleWidth;
    const auto perpendicularY = std::cos(angleRadians) * needleWidth;
    const auto needleX = centerX + std::cos(angleRadians) * needleLength;
    const auto needleY = centerY + std::sin(angleRadians) * needleLength;
    graphics.FillTriangle(IColor(255, 241, 193, 99), centerX + perpendicularX, centerY + perpendicularY,
                          centerX - perpendicularX, centerY - perpendicularY, needleX, needleY);
    graphics.DrawLine(IColor(255, 255, 232, 176), centerX, centerY, needleX, needleY, nullptr, 0.8F);

    const auto pointerX = centerX + std::cos(angleRadians) * radius * 0.62F;
    const auto pointerY = centerY + std::sin(angleRadians) * radius * 0.62F;
    IBlend pointerGlow(EBlend::Add, 0.55F);
    graphics.FillCircle(GetColor(kPR), pointerX, pointerY, 2.7F, &pointerGlow);
    graphics.FillCircle(IColor(255, 243, 255, 246), pointerX, pointerY, 1.15F);

    if (mMouseDown) {
      DrawSettingSparks(graphics, centerX, centerY, radius, angleRadians);
    }
  }

private:
  static constexpr float kPi = 3.14159265358979323846F;

  static void DrawArenaRing(IGraphics& graphics, float centerX, float centerY, float radius, float angle) {
    graphics.FillCircle(IColor(125, 0, 0, 0), centerX + 1.5F, centerY + 2.0F, radius * 0.94F);
    graphics.DrawCircle(IColor(220, 99, 81, 47), centerX, centerY, radius * 0.92F, nullptr, 1.2F);
    graphics.DrawCircle(IColor(180, 42, 44, 41), centerX, centerY, radius * 0.84F, nullptr, 1.0F);

    for (int tick = 0; tick < 24; ++tick) {
      const auto tickAngle = -kPi * 1.25F + static_cast<float>(tick) * (kPi * 1.5F / 23.0F);
      const auto active = tickAngle <= angle;
      const auto inner = radius * (tick % 4 == 0 ? 0.82F : 0.86F);
      const auto outer = radius * 0.91F;
      graphics.DrawLine(active ? IColor(255, 223, 171, 73) : IColor(160, 76, 70, 53),
                        centerX + std::cos(tickAngle) * inner, centerY + std::sin(tickAngle) * inner,
                        centerX + std::cos(tickAngle) * outer, centerY + std::sin(tickAngle) * outer,
                        nullptr, tick % 4 == 0 ? 1.25F : 0.75F);
    }
  }

  static void DrawSettingSparks(IGraphics& graphics, float centerX, float centerY, float radius, float angle) {
    for (int spark = 0; spark < 9; ++spark) {
      const auto spread = static_cast<float>(spark - 4) * 0.11F;
      const auto distance = radius * (0.80F + static_cast<float>(spark % 3) * 0.08F);
      const auto sparkAngle = angle + spread;
      const auto x = centerX + std::cos(sparkAngle) * distance;
      const auto y = centerY + std::sin(sparkAngle) * distance;
      IBlend glow(EBlend::Add, spark % 2 == 0 ? 0.65F : 0.40F);
      graphics.FillCircle(spark % 3 == 0 ? IColor(255, 111, 226, 183) : IColor(255, 246, 190, 86), x, y,
                          spark % 2 == 0 ? 1.45F : 0.9F, &glow);
    }
  }

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
