#pragma once

#include <array>
#include <cstddef>

namespace wfe::dsp {

enum class GoodbandCharacter {
  Clean = 0,
  Warm,
  Punch,
  Wide,
};

struct GoodbandParameters {
  float amount = 0.0F;
  GoodbandCharacter character = GoodbandCharacter::Clean;
  float mix = 1.0F;
  float outputTrimDb = 0.0F;
};

class GoodbandProcessor final {
 public:
  void Prepare(double sampleRate);
  void Reset() noexcept;
  void SetParameters(GoodbandParameters parameters) noexcept;

  [[nodiscard]] std::array<float, 2> ProcessFrame(float left, float right) noexcept;

 private:
  class OnePoleLowpass final {
   public:
    void Prepare(double sampleRate, double cutoffHz) noexcept;
    void Reset() noexcept;
    [[nodiscard]] float Process(float input) noexcept;

   private:
    float coefficient_ = 0.0F;
    float state_ = 0.0F;
  };

  class EnvelopeFollower final {
   public:
    void Prepare(double sampleRate) noexcept;
    void Reset() noexcept;
    [[nodiscard]] float Process(float input, float attackMs, float releaseMs) noexcept;

   private:
    double sampleRate_ = 48000.0;
    float envelope_ = 0.0F;
  };

  struct ChannelState {
    OnePoleLowpass lowSplit;
    OnePoleLowpass midSplit;
  };

  [[nodiscard]] static float Clamp(float value, float minimum, float maximum) noexcept;
  [[nodiscard]] static float DbToGain(float decibels) noexcept;
  [[nodiscard]] float ProcessBand(float input, float gain, float drive, float blend) const noexcept;

  std::array<ChannelState, 2> channels_{};
  std::array<EnvelopeFollower, 3> envelopes_{};
  GoodbandParameters parameters_{};
};

}  // namespace wfe::dsp
