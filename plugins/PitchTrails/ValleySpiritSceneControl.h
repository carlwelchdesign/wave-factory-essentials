#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "IControls.h"
#include "ValleySpiritVfxRenderer.h"

class ValleySpiritSceneControl final : public IControl {
public:
  ValleySpiritSceneControl(const IRECT& bounds, int delayParam, int pitchParam, int feedbackParam,
                           int diffusionParam, int mixParam, int freezeParam, int pathParam,
                           const IBitmap& vfxAtlas)
      : IControl(bounds, {delayParam, pitchParam, feedbackParam, diffusionParam, mixParam,
                          freezeParam, pathParam}),
        vfxAtlas_(vfxAtlas) {
    SetIgnoreMouse(true);
  }

  void OnInit() override { BeginMotionCycle(); }

  void Draw(IGraphics& graphics) override {
    const auto delay = static_cast<float>(GetValue(kDelayValue));
    const auto pitch = static_cast<float>(GetValue(kPitchValue));
    const auto feedback = static_cast<float>(GetValue(kFeedbackValue));
    const auto diffusion = static_cast<float>(GetValue(kDiffusionValue));
    const auto mix = static_cast<float>(GetValue(kMixValue));
    const auto freeze = static_cast<float>(GetValue(kFreezeValue));
    const auto path = static_cast<float>(GetValue(kPathValue));
    const auto intensity =
        std::clamp(0.16F + mix * 0.50F + freeze * 0.22F + responsePulse_ * 0.24F, 0.0F, 1.0F);

    DrawReturningRibbons(graphics, delay, pitch, feedback, diffusion,
                         std::clamp(intensity + path * 0.08F, 0.0F, 1.0F));
    DrawEchoFragments(graphics, delay, pitch, feedback,
                      std::clamp(diffusion + path * 0.12F, 0.0F, 1.0F), intensity);
  }

  void SetValueFromDelegate(double value, int valueIndex = 0) override {
    IControl::SetValueFromDelegate(value, valueIndex);
    responsePulse_ = 1.0F;
    SetDirty(false);
  }

  void OnEndAnimation() override {
    IControl::OnEndAnimation();
    BeginMotionCycle();
  }

  void OnRescale() override { vfxAtlas_ = GetUI()->GetScaledBitmap(vfxAtlas_); }

private:
  enum ValueIndex {
    kDelayValue = 0,
    kPitchValue,
    kFeedbackValue,
    kDiffusionValue,
    kMixValue,
    kFreezeValue,
    kPathValue,
  };

  static constexpr int kMotionCycleMs = 6400;
  static constexpr float kPi = 3.14159265358979323846F;
  static constexpr float kTau = kPi * 2.0F;
  static constexpr int kMaximumFragments = 52;

  void BeginMotionCycle() {
    cyclePhase_ = 0.0F;
    SetAnimation(
        [](IControl* control) {
          auto* scene = static_cast<ValleySpiritSceneControl*>(control);
          scene->cyclePhase_ =
              static_cast<float>(std::clamp(scene->GetAnimationProgress(), 0.0, 1.0));
          scene->responsePulse_ *= 0.955F;
          scene->SetDirty(false);
          if (scene->cyclePhase_ >= 1.0F) {
            scene->OnEndAnimation();
          }
        },
        kMotionCycleMs);
  }

  void DrawReturningRibbons(IGraphics& graphics, float delay, float pitch, float feedback,
                            float diffusion, float intensity) const {
    const auto startX = X(48.0F);
    const auto startY = Y(278.0F);
    const auto endX = X(508.0F);
    const auto endY = Y(213.0F) - (pitch - 0.5F) * Scale(64.0F);
    const auto breath = 0.88F + 0.12F * std::sin(cyclePhase_ * kTau);
    const auto threadCount = 2 + static_cast<int>(std::lround(diffusion * 4.0F));

    for (int thread = 0; thread < threadCount; ++thread) {
      const auto centered = static_cast<float>(thread) - static_cast<float>(threadCount - 1) * 0.5F;
      const auto spread = centered * Scale(4.0F + diffusion * 7.5F);
      const auto phaseOffset = centered * 0.31F + cyclePhase_ * kTau;
      const auto bend = Scale(32.0F + delay * 54.0F) *
                        (thread % 2 == 0 ? 1.0F : -0.72F);
      const auto control1X = startX + (endX - startX) * (0.25F + delay * 0.08F);
      const auto control1Y = startY - bend + spread + std::sin(phaseOffset) * Scale(8.0F);
      const auto control2X = startX + (endX - startX) * (0.68F - delay * 0.05F);
      const auto control2Y = endY + bend * 0.66F + spread;
      const auto opacity = intensity * breath * (0.34F + feedback * 0.42F) /
                           std::max(1.0F, 0.72F + static_cast<float>(threadCount) * 0.22F);
      const auto color = thread % 3 == 0 ? IColor(255, 172, 226, 255)
                                        : IColor(255, 138, 113, 235);

      DrawRibbonLayer(graphics, startX, startY + spread, control1X, control1Y, control2X,
                      control2Y, endX, endY + spread * 0.35F, color, opacity,
                      Scale(0.72F + diffusion * 0.28F));
    }
  }

  void DrawRibbonLayer(IGraphics& graphics, float startX, float startY, float control1X,
                       float control1Y, float control2X, float control2Y, float endX,
                       float endY, const IColor& color, float opacity, float width) const {
    constexpr std::array<float, 3> widths{6.8F, 3.0F, 1.0F};
    constexpr std::array<float, 3> alphas{0.12F, 0.38F, 0.92F};
    for (std::size_t layer = 0; layer < widths.size(); ++layer) {
      graphics.PathClear();
      graphics.PathMoveTo(startX, startY);
      graphics.PathCubicBezierTo(control1X, control1Y, control2X, control2Y, endX, endY);
      IBlend blend(EBlend::Add, std::clamp(opacity * alphas[layer], 0.0F, 1.0F));
      graphics.PathStroke(IPattern(color), width * widths[layer], IStrokeOptions(), &blend);
    }
  }

  void DrawEchoFragments(IGraphics& graphics, float delay, float pitch, float feedback,
                         float diffusion, float intensity) const {
    const auto count =
        std::clamp(12 + static_cast<int>(feedback * 40.0F), 12, kMaximumFragments);
    const auto pitchLift = (pitch - 0.5F) * Scale(82.0F);

    for (int index = 0; index < count; ++index) {
      const auto seedA = HashUnit(index, 0);
      const auto seedB = HashUnit(index, 1);
      const auto seedC = HashUnit(index, 2);
      const auto progress = std::fmod(cyclePhase_ * (0.42F + delay * 0.46F) + seedA, 1.0F);
      const auto eased = progress * progress * (3.0F - 2.0F * progress);
      const auto x = X(60.0F) + eased * Scale(445.0F);
      const auto arc = std::sin((eased + seedB * 0.12F) * kPi) *
                       Scale(30.0F + delay * 52.0F);
      const auto scatter = (seedB - 0.5F) * diffusion * Scale(56.0F);
      const auto y = Y(274.0F) - arc - eased * pitchLift + scatter;
      const auto lifecycle = std::sin(progress * kPi);
      const auto opacity = lifecycle * intensity * (0.28F + feedback * 0.62F);
      const auto size = Scale(6.0F + seedC * 15.0F + diffusion * 7.0F);
      const auto spriteRow = index % 6 == 0 ? 0 : (index % 3 == 0 ? 2 : 1);
      const auto spriteIndex = spriteRow * 4 + index % 4;
      const auto rotation = (seedA - 0.5F) * 110.0F + cyclePhase_ * 240.0F;
      valleyspirit::vfx::DrawAtlasSprite(graphics, vfxAtlas_, spriteIndex, x, y, size,
                                        rotation, opacity);
    }
  }

  static float HashUnit(int particleIndex, int channel) {
    auto value = static_cast<std::uint32_t>(particleIndex + 1) * 0x9E3779B9U;
    value ^= static_cast<std::uint32_t>(channel + 9) * 0x85EBCA6BU;
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

  float cyclePhase_ = 0.0F;
  float responsePulse_ = 0.0F;
  IBitmap vfxAtlas_;
};
