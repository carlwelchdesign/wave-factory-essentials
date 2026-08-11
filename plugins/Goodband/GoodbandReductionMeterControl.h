#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>

#include "IControls.h"

class GoodbandReductionMeterControl final : public IControl {
public:
  GoodbandReductionMeterControl(const IRECT& bounds,
                                const std::array<std::atomic<float>, 3>& reductionDb,
                                const std::atomic<float>& compensationDb,
                                int autoMatchParam)
      : IControl(bounds, autoMatchParam), reductionDb_(reductionDb), compensationDb_(compensationDb) {
    SetIgnoreMouse(true);
  }

  void OnInit() override { BeginMeterCycle(); }

  void Draw(IGraphics& graphics) override {
    constexpr std::array<float, 3> centers{88.0F, 205.0F, 322.0F};
    constexpr std::array<const char*, 3> labels{"LOW", "MID", "HIGH"};
    for (std::size_t index = 0; index < centers.size(); ++index) {
      const auto value = displayedReductionDb_[index];
      const auto normalized = std::clamp(value / 12.0F, 0.0F, 1.0F);
      const auto centerX = X(centers[index]);
      const auto centerY = Y(302.0F);
      const auto radius = Scale(58.0F);
      graphics.DrawArc(IColor(135, 62, 61, 54), centerX, centerY, radius, 138.0F, 402.0F,
                       nullptr, Scale(1.5F));
      if (normalized > 0.002F) {
        IBlend glow(EBlend::Add, 0.34F);
        graphics.DrawArc(IColor(255, 83, 229, 189), centerX, centerY, radius,
                         138.0F, 138.0F + 264.0F * normalized, &glow, Scale(3.4F));
        graphics.DrawArc(IColor(255, 231, 184, 87), centerX, centerY, radius,
                         138.0F, 138.0F + 264.0F * normalized, nullptr, Scale(1.15F));
      }

      char valueText[32]{};
      std::snprintf(valueText, sizeof(valueText), "%s  %.1f dB", labels[index], value);
      graphics.DrawText(IText(Scale(8.4F), IColor(230, 202, 184, 147), DEFAULT_FONT,
                              EAlign::Center, EVAlign::Middle),
                        valueText,
                        IRECT(X(centers[index] - 48.0F), Y(188.0F),
                              X(centers[index] + 48.0F), Y(201.0F)));
    }

    DrawGainCompensationStatus(graphics);
  }

  void OnEndAnimation() override {
    IControl::OnEndAnimation();
    BeginMeterCycle();
  }

private:
  static constexpr int kMeterCycleMs = 1000;

  void DrawGainCompensationStatus(IGraphics& graphics) const {
    const auto statusBounds = IRECT(X(414.0F), Y(109.0F), X(535.0F), Y(137.0F));
    const auto matchEnabled = GetValue() >= 0.5;
    const auto accent = matchEnabled ? IColor(255, 103, 226, 184)
                                     : IColor(210, 143, 123, 91);

    graphics.FillRect(IColor(218, 12, 14, 14), statusBounds);
    graphics.DrawRect(IColor(165, 116, 92, 59), statusBounds, nullptr, Scale(1.0F));
    graphics.FillRect(accent,
                      IRECT(statusBounds.L, statusBounds.T, statusBounds.L + Scale(2.0F), statusBounds.B));

    graphics.DrawText(IText(Scale(7.5F), IColor(220, 188, 169, 125), DEFAULT_FONT,
                            EAlign::Near, EVAlign::Middle),
                      "GAIN COMP",
                      IRECT(X(422.0F), Y(111.0F), X(477.0F), Y(121.0F)));

    char valueText[24]{};
    if (matchEnabled) {
      std::snprintf(valueText, sizeof(valueText), "%+.1f dB", displayedCompensationDb_);
    } else {
      std::snprintf(valueText, sizeof(valueText), "OFF");
    }
    graphics.DrawText(IText(Scale(10.5F), accent, DEFAULT_FONT,
                            EAlign::Near, EVAlign::Middle),
                      valueText,
                      IRECT(X(422.0F), Y(120.0F), X(529.0F), Y(134.0F)));
  }

  void BeginMeterCycle() {
    SetAnimation(
        [](IControl* control) {
          auto* meter = static_cast<GoodbandReductionMeterControl*>(control);
          for (std::size_t index = 0; index < meter->displayedReductionDb_.size(); ++index) {
            const auto target = meter->reductionDb_[index].load(std::memory_order_relaxed);
            const auto coefficient = target > meter->displayedReductionDb_[index] ? 0.42F : 0.12F;
            meter->displayedReductionDb_[index] +=
                coefficient * (target - meter->displayedReductionDb_[index]);
          }
          const auto compensation = meter->compensationDb_.load(std::memory_order_relaxed);
          meter->displayedCompensationDb_ += 0.18F * (compensation - meter->displayedCompensationDb_);
          meter->SetDirty(false);
          if (meter->GetAnimationProgress() >= 1.0) {
            meter->OnEndAnimation();
          }
        },
        kMeterCycleMs);
  }

  float X(float coordinate) const { return mRECT.L + coordinate * (mRECT.W() / 720.0F); }
  float Y(float coordinate) const { return mRECT.T + coordinate * (mRECT.H() / 440.0F); }
  float Scale(float amount) const {
    return amount * std::min(mRECT.W() / 720.0F, mRECT.H() / 440.0F);
  }

  const std::array<std::atomic<float>, 3>& reductionDb_;
  const std::atomic<float>& compensationDb_;
  std::array<float, 3> displayedReductionDb_{};
  float displayedCompensationDb_ = 0.0F;
};
