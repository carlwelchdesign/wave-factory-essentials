#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace wfe::dsp {

struct PitchTrailsParameters {
  float delayMs = 320.0F;
  float semitones = 7.0F;
  float feedback = 0.35F;
  float diffusion = 0.35F;
  float mix = 0.35F;
};

class PitchTrailsProcessor final {
 public:
  void Prepare(double sampleRate, double maximumDelaySeconds = 3.0);
  void Reset() noexcept;
  void SetParameters(PitchTrailsParameters parameters) noexcept;

  [[nodiscard]] std::array<float, 2> ProcessFrame(float left, float right) noexcept;

 private:
  class CircularDelay final {
   public:
    void Prepare(std::size_t capacity);
    void Reset() noexcept;
    void Write(float value) noexcept;
    [[nodiscard]] float Read(float delaySamples) const noexcept;

   private:
    std::vector<float> buffer_{};
    std::size_t writeIndex_ = 0;
  };

  class AllPass final {
   public:
    void Prepare(std::size_t delaySamples);
    void Reset() noexcept;
    [[nodiscard]] float Process(float input, float feedback) noexcept;

   private:
    std::vector<float> buffer_{};
    std::size_t index_ = 0;
  };

  struct ChannelState {
    CircularDelay delay;
    AllPass diffusionA;
    AllPass diffusionB;
    float feedbackMemory = 0.0F;
  };

  [[nodiscard]] static float Clamp(float value, float minimum, float maximum) noexcept;
  [[nodiscard]] float ReadPitched(const CircularDelay& delay, float phase) const noexcept;

  std::array<ChannelState, 2> channels_{};
  PitchTrailsParameters parameters_{};
  double sampleRate_ = 48000.0;
  float phase_ = 0.0F;
  float maximumDelaySamples_ = 144000.0F;
};

}  // namespace wfe::dsp
