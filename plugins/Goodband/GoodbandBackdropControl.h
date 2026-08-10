#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "IControls.h"

class GoodbandBackdropControl final : public IControl {
public:
  GoodbandBackdropControl(const IRECT& bounds, const IBitmap& restingBackground, const IBitmap& gestureBackground)
      : IControl(bounds), restingBackground_(restingBackground), gestureBackground_(gestureBackground) {
    SetIgnoreMouse(true);
  }

  void OnInit() override { BeginGestureCycle(); }

  void Draw(IGraphics& graphics) override {
    graphics.DrawBitmap(restingBackground_, mRECT);

    const auto gestureAmount = GestureAmount();
    if (gestureAmount > 0.0F) {
      IBlend gestureBlend(EBlend::Default, gestureAmount);
      graphics.DrawBitmap(gestureBackground_, mRECT, 1, &gestureBlend);
      DrawGestureEnergy(graphics, gestureAmount);
    }
  }

  void OnRescale() override {
    restingBackground_ = GetUI()->GetScaledBitmap(restingBackground_);
    gestureBackground_ = GetUI()->GetScaledBitmap(gestureBackground_);
  }

  void OnEndAnimation() override {
    IControl::OnEndAnimation();
    BeginGestureCycle();
  }

private:
  static constexpr int kGestureCycleDurationMs = 60000;
  static constexpr float kGestureWindowStart = 0.95F;
  static constexpr float kPi = 3.14159265358979323846F;
  static constexpr float kTau = kPi * 2.0F;
  static constexpr int kGestureParticleCount = 72;

  void BeginGestureCycle() {
    cycleProgress_ = 0.0;
    SetAnimation(
        [](IControl* control) {
          auto* backdrop = static_cast<GoodbandBackdropControl*>(control);
          backdrop->cycleProgress_ = std::clamp(backdrop->GetAnimationProgress(), 0.0, 1.0);
          backdrop->SetDirty(false);
          if (backdrop->cycleProgress_ >= 1.0) {
            backdrop->OnEndAnimation();
          }
        },
        kGestureCycleDurationMs);
  }

  float GestureAmount() const {
    const auto progress = static_cast<float>(cycleProgress_);
    if (progress < kGestureWindowStart) {
      return 0.0F;
    }
    const auto local = std::clamp((progress - kGestureWindowStart) / (1.0F - kGestureWindowStart), 0.0F, 1.0F);
    return std::pow(std::sin(local * kPi), 1.35F);
  }

  static float HashUnit(int particleIndex, int channel) {
    auto value = static_cast<std::uint32_t>(particleIndex + 1) * 0x9E3779B9U;
    value ^= static_cast<std::uint32_t>(channel + 5) * 0x85EBCA6BU;
    value ^= value >> 16U;
    value *= 0x7FEB352DU;
    value ^= value >> 15U;
    return static_cast<float>(value & 0x00FFFFFFU) / static_cast<float>(0x01000000U);
  }

  void DrawGestureEnergy(IGraphics& graphics, float gestureAmount) const {
    const auto centerX = X(566.0F);
    const auto centerY = Y(232.0F);
    const auto orbitPhase = static_cast<float>(cycleProgress_) * kTau * 5.0F;

    for (int index = 0; index < kGestureParticleCount; ++index) {
      const auto seedA = HashUnit(index, 0);
      const auto seedB = HashUnit(index, 1);
      const auto seedC = HashUnit(index, 2);
      const auto angle = seedA * kTau + orbitPhase * (0.45F + seedC * 0.8F);
      const auto distance = Scale(12.0F + seedB * (38.0F + gestureAmount * 62.0F));
      const auto x = centerX + std::cos(angle) * distance;
      const auto y = centerY + std::sin(angle) * distance * 0.62F;
      const auto radius = Scale(1.0F + seedC * 2.8F + gestureAmount * 1.8F);
      const auto color = index % 6 == 0 ? IColor(255, 255, 235, 174) : IColor(255, 86, 226, 190);
      IBlend outerGlow(EBlend::Add, gestureAmount * 0.16F);
      IBlend innerGlow(EBlend::Add, gestureAmount * 0.72F);
      graphics.FillCircle(color, x, y, radius * 5.2F, &outerGlow);
      graphics.FillCircle(color, x, y, radius, &innerGlow);
    }

    IBlend auraBlend(EBlend::Add, gestureAmount * 0.18F);
    graphics.FillCircle(IColor(255, 118, 235, 198), centerX, centerY, Scale(28.0F + gestureAmount * 18.0F),
                        &auraBlend);
  }

  float X(float coordinate) const { return mRECT.L + coordinate * (mRECT.W() / 720.0F); }
  float Y(float coordinate) const { return mRECT.T + coordinate * (mRECT.H() / 440.0F); }
  float Scale(float amount) const { return amount * std::min(mRECT.W() / 720.0F, mRECT.H() / 440.0F); }

  IBitmap restingBackground_;
  IBitmap gestureBackground_;
  double cycleProgress_ = 0.0;
};
