#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "IControls.h"
#include "ChiVfxRenderer.h"

class GoodbandSceneControl final : public IControl {
public:
  GoodbandSceneControl(const IRECT& bounds, int amountParam, int characterParam, int mixParam, int outputParam,
                       const IBitmap& vfxAtlas)
      : IControl(bounds, {amountParam, characterParam, mixParam, outputParam}), vfxAtlas_(vfxAtlas) {
    SetIgnoreMouse(true);
  }

  void Draw(IGraphics& graphics) override {
    graphics.FillRect(IColor(255, 174, 132, 64), IRECT(X(38.0F), Y(108.0F), X(101.0F), Y(110.0F)));

    const auto amount = static_cast<float>(current_[kAmountValue]);
    const auto mix = static_cast<float>(current_[kMixValue]);
    const auto output = static_cast<float>(current_[kOutputValue]);
    const auto energy = std::clamp(amount * mix, 0.0F, 1.0F);
    const auto character = CharacterIndex();
    const auto handX = X(431.0F);
    const auto handY = Y(139.0F);

    DrawRestingConstellation(graphics, character, energy, output, handX, handY);
    DrawChiRibbons(graphics, character, energy, handX, handY);
    DrawMagicParticles(graphics, character, amount, mix, output, handX, handY);
  }

  void OnRescale() override {
    IControl::OnRescale();
    vfxAtlas_ = GetUI()->GetScaledBitmap(vfxAtlas_);
  }

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
          static_cast<GoodbandSceneControl*>(control)->AdvanceAnimation();
        },
        kAnimationDurationMs);
  }

private:
  enum ValueIndex { kAmountValue = 0, kCharacterValue, kMixValue, kOutputValue, kValueCount };
  static constexpr int kAnimationDurationMs = 920;
  static constexpr float kPi = 3.14159265358979323846F;
  static constexpr float kTau = kPi * 2.0F;
  static constexpr int kMaximumAnimatedParticles = 64;

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

  static float HashUnit(int particleIndex, int channel, int character) {
    auto value = static_cast<std::uint32_t>(particleIndex + 1) * 0x9E3779B9U;
    value ^= static_cast<std::uint32_t>(channel + 7) * 0x85EBCA6BU;
    value ^= static_cast<std::uint32_t>(character + 11) * 0xC2B2AE35U;
    value ^= value >> 16U;
    value *= 0x7FEB352DU;
    value ^= value >> 15U;
    return static_cast<float>(value & 0x00FFFFFFU) / static_cast<float>(0x01000000U);
  }

  IColor ParticleColor(int character, int particleIndex) const {
    const auto highlight = particleIndex % 5 == 0;
    switch (character) {
      case 1:
        return highlight ? IColor(255, 255, 239, 190) : IColor(255, 224, 171, 72);
      case 2:
        return highlight ? IColor(255, 255, 225, 160) : IColor(255, 232, 91, 55);
      case 3:
        return highlight ? IColor(255, 222, 252, 246) : IColor(255, 76, 212, 190);
      default:
        return highlight ? IColor(255, 235, 250, 230) : IColor(255, 118, 205, 157);
    }
  }

  void DrawRestingConstellation(IGraphics& graphics, int character, float energy, float output, float handX,
                                float handY) const {
    constexpr int restingParticles = 16;
    for (int index = 0; index < restingParticles; ++index) {
      const auto angle = HashUnit(index, 0, character) * kTau;
      const auto distance = Scale(13.0F + HashUnit(index, 1, character) * (22.0F + energy * 18.0F));
      const auto x = handX + std::cos(angle) * distance;
      const auto y = handY + std::sin(angle) * distance * 0.72F;
      const auto size = Scale(5.0F + HashUnit(index, 2, character) * 7.0F + output * 2.0F);
      const auto spriteRow = character == 1 ? 2 : (character == 3 ? 1 : 3);
      const auto spriteIndex = spriteRow * 4 + index % 4;
      const auto rotation = angle * 180.0F / kPi + HashUnit(index, 3, character) * 90.0F;
      threefold::vfx::DrawAtlasSprite(graphics, vfxAtlas_, spriteIndex, x, y, size, rotation,
                                     0.20F + energy * 0.30F);
    }
  }

  void DrawChiRibbons(IGraphics& graphics, int character, float energy, float handX, float handY) const {
    const auto lifecycle = static_cast<float>(std::sin(animationPhase_ * kPi));
    if (lifecycle <= 0.001F) {
      return;
    }

    const auto intensity = lifecycle * (0.34F + energy * 0.66F);
    const auto jade = ParticleColor(character == 2 ? 2 : 3, 1);
    const auto gold = ParticleColor(1, 0);
    const auto horizontalDirection = character == 3 ? -1.0F : 1.0F;
    threefold::vfx::DrawChiRibbon(graphics, handX, handY, handX - Scale(148.0F) * horizontalDirection,
                                 handY + Scale(72.0F), Scale(31.0F), jade, intensity, Scale(0.72F));
    threefold::vfx::DrawChiRibbon(graphics, handX + Scale(3.0F), handY - Scale(4.0F),
                                 handX - Scale(102.0F), handY - Scale(54.0F), -Scale(24.0F), gold,
                                 intensity * 0.82F, Scale(0.52F));
  }

  void DrawMagicParticles(IGraphics& graphics, int character, float amount, float mix, float output, float handX,
                          float handY) const {
    const auto count = std::clamp(24 + static_cast<int>(amount * 60.0F), 24, kMaximumAnimatedParticles);
    const auto masterOpacity = std::clamp(0.44F + mix * 0.56F, 0.0F, 1.0F);

    for (int index = 0; index < count; ++index) {
      const auto delay = HashUnit(index, 3, character) * 0.24F;
      const auto localProgress = std::clamp(
          static_cast<float>((animationPhase_ - delay) / std::max(0.01, 1.0 - static_cast<double>(delay))),
          0.0F, 1.0F);
      if (localProgress <= 0.0F || localProgress >= 1.0F) {
        continue;
      }

      const auto seedA = HashUnit(index, 0, character);
      const auto seedB = HashUnit(index, 1, character);
      const auto seedC = HashUnit(index, 2, character);
      const auto angle = seedA * kTau;
      float x = handX;
      float y = handY;

      PositionParticle(character, angle, seedB, seedC, localProgress, handX, handY, x, y);

      const auto lifecycle = std::sin(localProgress * kPi);
      const auto size = Scale(8.0F + seedC * 16.0F + amount * 6.0F + output * 2.0F);
      const auto spriteRow = index % 7 == 0 ? 0 : (character == 1 ? 2 : (character == 3 ? 1 : 3));
      const auto spriteIndex = spriteRow * 4 + index % 4;
      const auto rotation = angle * 180.0F / kPi + localProgress * (90.0F + seedB * 210.0F);
      threefold::vfx::DrawAtlasSprite(graphics, vfxAtlas_, spriteIndex, x, y, size, rotation,
                                     lifecycle * masterOpacity * 0.92F);
    }
  }

  void PositionParticle(int character, float angle, float seedB, float seedC, float progress, float handX,
                        float handY, float& x, float& y) const {
    switch (character) {
      case 1: {
        const auto rise = Scale(progress * (52.0F + seedB * 92.0F));
        x = handX + std::cos(angle) * Scale(10.0F + seedC * 42.0F) + std::sin(progress * kPi) * Scale(14.0F);
        y = handY - rise + std::sin(angle) * Scale(14.0F);
        break;
      }
      case 2: {
        const auto distance = Scale(progress * (78.0F + seedB * 128.0F));
        x = handX + std::cos(angle) * distance;
        y = handY + std::sin(angle) * distance;
        break;
      }
      case 3: {
        const auto direction = seedB < 0.5F ? -1.0F : 1.0F;
        x = handX + direction * Scale(progress * (90.0F + seedC * 172.0F));
        y = handY + std::sin(angle + progress * kPi) * Scale(16.0F + seedB * 36.0F);
        break;
      }
      default: {
        const auto orbit = angle + progress * (1.4F + seedB * 1.8F);
        const auto distance = Scale(14.0F + seedC * 48.0F + progress * 28.0F);
        x = handX + std::cos(orbit) * distance;
        y = handY + std::sin(orbit) * distance * 0.72F;
        break;
      }
    }
  }

  float X(float coordinate) const { return mRECT.L + coordinate * (mRECT.W() / 720.0F); }
  float Y(float coordinate) const { return mRECT.T + coordinate * (mRECT.H() / 440.0F); }
  float Scale(float amount) const { return amount * std::min(mRECT.W() / 720.0F, mRECT.H() / 440.0F); }

  std::array<double, kValueCount> current_{};
  std::array<double, kValueCount> start_{};
  std::array<double, kValueCount> target_{};
  double animationPhase_ = 1.0;
  IBitmap vfxAtlas_;
};
