#pragma once

#include <algorithm>
#include <cmath>

#include "IControls.h"

class IllustratedSpiritDialControl final : public IVKnobControl {
public:
  IllustratedSpiritDialControl(const IRECT& bounds, int paramIndex, const char* label, const IVStyle& style,
                               const IBitmap& hardware, float hardwareScale)
      : IVKnobControl(bounds, paramIndex, label, style, true),
        hardware_(hardware),
        hardwareScale_(hardwareScale) {}

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
    const auto hardwareRadius = radius * hardwareScale_;
    const auto hardwareBounds =
        IRECT(centerX - hardwareRadius, centerY - hardwareRadius, centerX + hardwareRadius,
              centerY + hardwareRadius);

    graphics.DrawFittedBitmap(hardware_, hardwareBounds);
    DrawNeedle(graphics, centerX, centerY, radius, angleRadians);

    if (mMouseDown) {
      DrawSettingEnergy(graphics, centerX, centerY, radius, angleRadians);
    }
  }

  void OnRescale() override {
    IVKnobControl::OnRescale();
    hardware_ = GetUI()->GetScaledBitmap(hardware_);
  }

private:
  static constexpr float kPi = 3.14159265358979323846F;

  static void DrawNeedle(IGraphics& graphics, float centerX, float centerY, float radius, float angle) {
    const auto length = radius * 0.57F;
    const auto tipX = centerX + std::cos(angle) * length;
    const auto tipY = centerY + std::sin(angle) * length;
    const auto width = std::max(1.8F, radius * 0.045F);
    const auto normalX = -std::sin(angle) * width;
    const auto normalY = std::cos(angle) * width;

    IBlend glow(EBlend::Add, 0.38F);
    graphics.FillTriangle(IColor(255, 196, 224, 255), centerX + normalX * 2.1F,
                          centerY + normalY * 2.1F, centerX - normalX * 2.1F,
                          centerY - normalY * 2.1F, tipX, tipY, &glow);
    graphics.FillTriangle(IColor(255, 231, 220, 188), centerX + normalX, centerY + normalY,
                          centerX - normalX, centerY - normalY, tipX, tipY);
    graphics.DrawLine(IColor(255, 255, 250, 229), centerX, centerY, tipX, tipY, nullptr, 0.7F);
    graphics.FillCircle(IColor(255, 32, 27, 26), centerX, centerY, radius * 0.095F);
    graphics.DrawCircle(IColor(255, 190, 151, 92), centerX, centerY, radius * 0.095F, nullptr, 1.0F);
  }

  static void DrawSettingEnergy(IGraphics& graphics, float centerX, float centerY, float radius,
                                float angle) {
    for (int spark = 0; spark < 7; ++spark) {
      const auto spread = static_cast<float>(spark - 3) * 0.12F;
      const auto sparkAngle = angle + spread;
      const auto distance = radius * (0.88F + static_cast<float>(spark % 2) * 0.09F);
      const auto x = centerX + std::cos(sparkAngle) * distance;
      const auto y = centerY + std::sin(sparkAngle) * distance;
      const auto size = spark % 2 == 0 ? 2.1F : 1.35F;
      IBlend glow(EBlend::Add, spark % 2 == 0 ? 0.78F : 0.48F);
      graphics.FillTriangle(IColor(255, 171, 235, 255), x, y - size, x + size * 0.72F,
                            y + size * 0.55F, x - size * 0.72F, y + size * 0.55F, &glow);
    }
  }

  IBitmap hardware_;
  float hardwareScale_ = 1.0F;
};
