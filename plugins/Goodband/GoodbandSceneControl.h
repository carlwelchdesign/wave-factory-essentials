#pragma once

#include <algorithm>
#include <array>
#include <cmath>

#include "IControls.h"

class GoodbandSceneControl final : public IControl {
public:
  GoodbandSceneControl(const IRECT& bounds, const IBitmap& background, int amountParam, int characterParam,
                       int mixParam, int outputParam)
      : IControl(bounds, {amountParam, characterParam, mixParam, outputParam}), background_(background) {
    SetIgnoreMouse(true);
  }

  void Draw(IGraphics& graphics) override {
    graphics.DrawBitmap(background_, mRECT);

    const auto panel = IRECT(X(16.0F), Y(18.0F), X(374.0F), Y(420.0F));
    graphics.FillRoundRect(IColor(188, 10, 14, 16), panel, 18.0F);
    graphics.DrawRoundRect(IColor(110, 119, 231, 198), panel, 18.0F, nullptr, 1.0F);

    const auto amount = static_cast<float>(current_[kAmountValue]);
    const auto character = CharacterIndex();
    const auto mix = static_cast<float>(current_[kMixValue]);
    const auto output = static_cast<float>(current_[kOutputValue]);
    const auto energy = std::clamp(amount * mix, 0.0F, 1.0F);
    const auto pulse = static_cast<float>(std::sin(animationPhase_ * 3.14159265358979323846));
    const auto color = CharacterColor(character);
    const auto handX = X(432.0F);
    const auto handY = Y(220.0F);
    const auto radius = Scale(18.0F + (energy * 34.0F) + (pulse * 10.0F));

    IBlend glowBlend(EBlend::Default, 0.08F + energy * 0.18F);
    graphics.FillCircle(color, handX, handY, radius * 1.35F, &glowBlend);

    IBlend ringBlend(EBlend::Default, 0.30F + energy * 0.65F);
    graphics.DrawCircle(color, handX, handY, radius, &ringBlend, Scale(1.4F + energy * 2.2F));

    DrawEnergyTrail(graphics, color, energy, character, pulse);
    DrawCharacterGesture(graphics, color, energy, character, pulse, handX, handY, radius);

    const auto gongStrength = std::clamp(0.12F + output * 0.30F + energy * 0.22F, 0.0F, 0.72F);
    IBlend gongBlend(EBlend::Default, gongStrength);
    graphics.DrawCircle(CharacterColor(character), X(300.0F), Y(158.0F), Scale(82.0F + output * 10.0F),
                        &gongBlend, Scale(1.0F + output * 1.5F));
  }

  void OnRescale() override { background_ = GetUI()->GetScaledBitmap(background_); }

  void SetValueFromDelegate(double value, int valueIndex = 0) override {
    IControl::SetValueFromDelegate(value, valueIndex);
    if (valueIndex < 0 || valueIndex >= kValueCount) {
      return;
    }

    start_ = current_;
    target_[static_cast<std::size_t>(valueIndex)] = std::clamp(value, 0.0, 1.0);
    animationPhase_ = 0.0;
    SetAnimation(
        [](IControl* control) {
          auto* scene = static_cast<GoodbandSceneControl*>(control);
          scene->AdvanceAnimation();
        },
        kAnimationDurationMs);
  }

private:
  enum ValueIndex { kAmountValue = 0, kCharacterValue, kMixValue, kOutputValue, kValueCount };
  static constexpr int kAnimationDurationMs = 260;

  void AdvanceAnimation() {
    const auto progress = std::clamp(GetAnimationProgress(), 0.0, 1.0);
    const auto eased = 1.0 - std::pow(1.0 - progress, 3.0);
    for (std::size_t index = 0; index < current_.size(); ++index) {
      current_[index] = start_[index] + (target_[index] - start_[index]) * eased;
    }
    animationPhase_ = progress;
    SetDirty(false);

    if (progress >= 1.0) {
      OnEndAnimation();
    }
  }

  int CharacterIndex() const {
    return std::clamp(static_cast<int>(std::lround(target_[kCharacterValue] * 3.0)), 0, 3);
  }

  IColor CharacterColor(int character) const {
    switch (character) {
      case 1:
        return IColor(255, 226, 181, 94);
      case 2:
        return IColor(255, 239, 112, 82);
      case 3:
        return IColor(255, 100, 218, 232);
      default:
        return IColor(255, 82, 232, 191);
    }
  }

  void DrawEnergyTrail(IGraphics& graphics, const IColor& color, float energy, int character, float pulse) const {
    const auto amplitude = Scale((character == 3 ? 24.0F : 12.0F) + energy * (character == 3 ? 34.0F : 20.0F));
    const auto thickness = Scale(1.0F + energy * 2.2F);
    const auto verticalCenter = Y(252.0F);
    IBlend blend(EBlend::Default, 0.22F + energy * 0.62F);

    constexpr int segments = 44;
    for (int segment = 0; segment < segments; ++segment) {
      const auto from = static_cast<float>(segment) / static_cast<float>(segments);
      const auto to = static_cast<float>(segment + 1) / static_cast<float>(segments);
      const auto x1 = X(22.0F + from * 430.0F);
      const auto x2 = X(22.0F + to * 430.0F);
      const auto cycles = character == 2 ? 4.0F : 2.0F;
      const auto y1 = verticalCenter + std::sin((from * cycles + pulse * 0.22F) * 6.283185307F) * amplitude;
      const auto y2 = verticalCenter + std::sin((to * cycles + pulse * 0.22F) * 6.283185307F) * amplitude;
      graphics.DrawLine(color, x1, y1, x2, y2, &blend, thickness);
    }
  }

  void DrawCharacterGesture(IGraphics& graphics, const IColor& color, float energy, int character, float pulse,
                            float handX, float handY, float radius) const {
    if (character == 1) {
      IBlend warmth(EBlend::Default, 0.10F + energy * 0.22F);
      graphics.FillCircle(color, handX, handY, radius * 1.9F, &warmth);
      return;
    }

    if (character == 2) {
      IBlend impact(EBlend::Default, 0.25F + pulse * 0.70F);
      for (int ray = 0; ray < 10; ++ray) {
        const auto angle = static_cast<float>(ray) * 0.628318531F;
        const auto inner = radius * 1.2F;
        const auto outer = inner + Scale(10.0F + pulse * 28.0F);
        graphics.DrawLine(color, handX + std::cos(angle) * inner, handY + std::sin(angle) * inner,
                          handX + std::cos(angle) * outer, handY + std::sin(angle) * outer, &impact,
                          Scale(1.0F + energy * 2.0F));
      }
      return;
    }

    if (character == 3) {
      IBlend widthBlend(EBlend::Default, 0.20F + energy * 0.58F);
      graphics.DrawArc(color, handX, handY, radius * 1.7F, 200.0F, 520.0F, &widthBlend,
                       Scale(1.0F + energy * 2.0F));
      graphics.DrawArc(color, handX, handY, radius * 2.2F, 210.0F, 510.0F, &widthBlend,
                       Scale(0.8F + energy * 1.6F));
    }
  }

  float X(float coordinate) const { return mRECT.L + coordinate * (mRECT.W() / 720.0F); }
  float Y(float coordinate) const { return mRECT.T + coordinate * (mRECT.H() / 440.0F); }
  float Scale(float amount) const { return amount * std::min(mRECT.W() / 720.0F, mRECT.H() / 440.0F); }

  IBitmap background_;
  std::array<double, kValueCount> current_{};
  std::array<double, kValueCount> start_{};
  std::array<double, kValueCount> target_{};
  double animationPhase_ = 1.0;
};
