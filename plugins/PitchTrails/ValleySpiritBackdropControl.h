#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "IControls.h"
#include "ValleySpiritVfxRenderer.h"

class ValleySpiritBackdropControl final : public IControl {
public:
  ValleySpiritBackdropControl(const IRECT& bounds, int delayParam, int pitchParam,
                              int feedbackParam, int diffusionParam, int mixParam,
                              const IBitmap& restingBackground,
                              const IBitmap& gestureBackground, const IBitmap& vfxAtlas)
      : IControl(bounds, {delayParam, pitchParam, feedbackParam, diffusionParam, mixParam}),
        restingBackground_(restingBackground),
        gestureBackground_(gestureBackground),
        vfxAtlas_(vfxAtlas) {
    SetIgnoreMouse(true);
  }

  void OnInit() override { BeginGestureCycle(); }

  void Draw(IGraphics& graphics) override {
    graphics.DrawBitmap(restingBackground_, mRECT);

    const auto gestureAmount = GestureAmount();
    if (gestureAmount > 0.0F) {
      IBlend gestureBlend(EBlend::Default, gestureAmount);
      graphics.DrawBitmap(gestureBackground_, mRECT, 1, &gestureBlend);
    }

    const auto feedback = static_cast<float>(GetValue(kFeedbackValue));
    const auto diffusion = static_cast<float>(GetValue(kDiffusionValue));
    const auto mix = static_cast<float>(GetValue(kMixValue));
    const auto energy = std::clamp(gestureAmount * (0.72F + mix * 0.52F) +
                                       responsePulse_ * 0.42F,
                                   0.0F, 1.0F);
    if (energy > 0.015F) {
      DrawGestureEnergy(graphics, gestureAmount, feedback, diffusion, mix, energy);
    }
  }

  void SetValueFromDelegate(double value, int valueIndex = 0) override {
    IControl::SetValueFromDelegate(value, valueIndex);
    responsePulse_ = 1.0F;
    SetDirty(false);
  }

  void OnRescale() override {
    restingBackground_ = GetUI()->GetScaledBitmap(restingBackground_);
    gestureBackground_ = GetUI()->GetScaledBitmap(gestureBackground_);
    vfxAtlas_ = GetUI()->GetScaledBitmap(vfxAtlas_);
  }

  void OnEndAnimation() override {
    IControl::OnEndAnimation();
    BeginGestureCycle();
  }

private:
  enum ValueIndex {
    kDelayValue = 0,
    kPitchValue,
    kFeedbackValue,
    kDiffusionValue,
    kMixValue,
  };

  static constexpr int kGestureCycleDurationMs = 12000;
  static constexpr float kGestureWindowStart = 0.62F;
  static constexpr float kPi = 3.14159265358979323846F;
  static constexpr float kTau = kPi * 2.0F;
  static constexpr int kGestureParticleCount = 58;

  void BeginGestureCycle() {
    cycleProgress_ = 0.0;
    SetAnimation(
        [](IControl* control) {
          auto* backdrop = static_cast<ValleySpiritBackdropControl*>(control);
          backdrop->cycleProgress_ =
              std::clamp(backdrop->GetAnimationProgress(), 0.0, 1.0);
          backdrop->responsePulse_ *= 0.93F;
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
    const auto local =
        std::clamp((progress - kGestureWindowStart) / (1.0F - kGestureWindowStart), 0.0F, 1.0F);
    const auto envelope = std::sin(local * kPi);
    return envelope * envelope * (3.0F - 2.0F * envelope);
  }

  void DrawGestureEnergy(IGraphics& graphics, float gestureAmount, float feedback,
                         float diffusion, float mix, float energy) const {
    const auto handX = X(488.0F - gestureAmount * 63.0F);
    const auto handY = Y(224.0F - gestureAmount * 35.0F);
    const auto orbitPhase = static_cast<float>(cycleProgress_) * kTau * 4.2F;
    const auto count = std::clamp(22 + static_cast<int>(feedback * 36.0F), 22,
                                  kGestureParticleCount);

    for (int index = 0; index < count; ++index) {
      const auto seedA = HashUnit(index, 0);
      const auto seedB = HashUnit(index, 1);
      const auto seedC = HashUnit(index, 2);
      const auto angle = seedA * kTau + orbitPhase * (0.38F + seedC * 0.92F);
      const auto distance =
          Scale(8.0F + seedB * (24.0F + diffusion * 54.0F + gestureAmount * 28.0F));
      const auto x = handX + std::cos(angle) * distance;
      const auto y = handY + std::sin(angle) * distance * (0.54F + diffusion * 0.28F);
      const auto spriteRow = index % 7 == 0 ? 0 : (index % 3 == 0 ? 2 : 1);
      const auto spriteIndex = spriteRow * 4 + index % 4;
      const auto size = Scale(8.0F + seedC * 18.0F + mix * 9.0F);
      const auto rotation = angle * 180.0F / kPi + orbitPhase * 15.0F;
      valleyspirit::vfx::DrawAtlasSprite(graphics, vfxAtlas_, spriteIndex, x, y, size,
                                        rotation, energy * (0.46F + seedB * 0.68F));
    }

    valleyspirit::vfx::DrawSpiritRibbon(
        graphics, X(86.0F), Y(284.0F), handX, handY, Scale(58.0F + diffusion * 30.0F),
        IColor(255, 137, 126, 255), energy * 0.92F, Scale(0.92F));
    valleyspirit::vfx::DrawSpiritRibbon(
        graphics, X(236.0F), Y(356.0F), handX, handY, -Scale(44.0F + feedback * 24.0F),
        IColor(255, 174, 230, 255), energy * 0.76F, Scale(0.72F));
  }

  static float HashUnit(int particleIndex, int channel) {
    auto value = static_cast<std::uint32_t>(particleIndex + 1) * 0x9E3779B9U;
    value ^= static_cast<std::uint32_t>(channel + 13) * 0x85EBCA6BU;
    value ^= value >> 16U;
    value *= 0x7FEB352DU;
    value ^= value >> 15U;
    return static_cast<float>(value & 0x00FFFFFFU) /
           static_cast<float>(0x01000000U);
  }

  float X(float coordinate) const { return mRECT.L + coordinate * (mRECT.W() / 760.0F); }
  float Y(float coordinate) const { return mRECT.T + coordinate * (mRECT.H() / 460.0F); }
  float Scale(float amount) const {
    return amount * std::min(mRECT.W() / 760.0F, mRECT.H() / 460.0F);
  }

  IBitmap restingBackground_;
  IBitmap gestureBackground_;
  IBitmap vfxAtlas_;
  double cycleProgress_ = 0.0;
  float responsePulse_ = 0.0F;
};
