#pragma once

#include <algorithm>
#include <cmath>

#include "IControls.h"

class IllustratedShurikenKnobControl final : public IVKnobControl {
public:
  IllustratedShurikenKnobControl(const IRECT& bounds, int paramIndex, const char* label, const IVStyle& style,
                                 const IBitmap& arena, const IBitmap& shuriken)
      : IVKnobControl(bounds, paramIndex, label, style, true), arena_(arena), shuriken_(shuriken) {}

  void Draw(IGraphics& graphics) override {
    DrawLabel(graphics);
    DrawWidget(graphics);
    DrawValue(graphics, mValueMouseOver);
  }

  void DrawWidget(IGraphics& graphics) override {
    const auto radius = GetRadius();
    const auto centerX = mWidgetBounds.MW();
    const auto centerY = mWidgetBounds.MH();
    const auto angleDegrees = mAngle1 + static_cast<float>(GetValue()) * (mAngle2 - mAngle1);
    const auto angleRadians = (angleDegrees - 90.0F) * kPi / 180.0F;

    graphics.DrawBitmap(arena_, mWidgetBounds.GetScaledAboutCentre(1.25F));
    graphics.DrawBitmap(shuriken_, mWidgetBounds.GetScaledAboutCentre(0.93F));
    DrawNeedle(graphics, centerX, centerY, radius, angleRadians);

    if (mMouseDown) {
      DrawSettingSparks(graphics, centerX, centerY, radius, angleRadians);
    }
  }

  void OnRescale() override {
    IVKnobControl::OnRescale();
    arena_ = GetUI()->GetScaledBitmap(arena_);
    shuriken_ = GetUI()->GetScaledBitmap(shuriken_);
  }

private:
  static constexpr float kPi = 3.14159265358979323846F;

  static void DrawNeedle(IGraphics& graphics, float centerX, float centerY, float radius, float angle) {
    const auto needleLength = radius * 0.63F;
    const auto needleWidth = std::max(2.4F, radius * 0.055F);
    const auto perpendicularX = -std::sin(angle) * needleWidth;
    const auto perpendicularY = std::cos(angle) * needleWidth;
    const auto needleX = centerX + std::cos(angle) * needleLength;
    const auto needleY = centerY + std::sin(angle) * needleLength;

    IBlend glow(EBlend::Add, 0.28F);
    graphics.FillTriangle(IColor(255, 243, 193, 95), centerX + perpendicularX * 1.8F,
                          centerY + perpendicularY * 1.8F, centerX - perpendicularX * 1.8F,
                          centerY - perpendicularY * 1.8F, needleX, needleY, &glow);
    graphics.FillTriangle(IColor(255, 245, 204, 122), centerX + perpendicularX, centerY + perpendicularY,
                          centerX - perpendicularX, centerY - perpendicularY, needleX, needleY);
    graphics.DrawLine(IColor(255, 255, 239, 194), centerX, centerY, needleX, needleY, nullptr, 0.75F);
    graphics.FillCircle(IColor(255, 31, 28, 22), centerX, centerY, radius * 0.12F);
    graphics.DrawCircle(IColor(255, 228, 178, 80), centerX, centerY, radius * 0.12F, nullptr, 1.2F);
  }

  static void DrawSettingSparks(IGraphics& graphics, float centerX, float centerY, float radius, float angle) {
    for (int spark = 0; spark < 9; ++spark) {
      const auto spread = static_cast<float>(spark - 4) * 0.11F;
      const auto distance = radius * (0.90F + static_cast<float>(spark % 3) * 0.07F);
      const auto sparkAngle = angle + spread;
      const auto x = centerX + std::cos(sparkAngle) * distance;
      const auto y = centerY + std::sin(sparkAngle) * distance;
      IBlend glow(EBlend::Add, spark % 2 == 0 ? 0.68F : 0.42F);
      graphics.FillCircle(spark % 3 == 0 ? IColor(255, 103, 226, 184) : IColor(255, 248, 189, 79), x, y,
                          spark % 2 == 0 ? 1.4F : 0.85F, &glow);
    }
  }

  IBitmap arena_;
  IBitmap shuriken_;
};
