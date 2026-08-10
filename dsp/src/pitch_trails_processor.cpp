#include "wfe/dsp/pitch_trails_processor.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace wfe::dsp {
namespace {

constexpr float kPi = 3.14159265358979323846F;

[[nodiscard]] float WrapPhase(float phase) noexcept {
  phase -= std::floor(phase);
  return phase < 0.0F ? phase + 1.0F : phase;
}

[[nodiscard]] float Hann(float phase) noexcept {
  return 0.5F - 0.5F * std::cos(2.0F * kPi * WrapPhase(phase));
}

}  // namespace

void PitchTrailsProcessor::CircularDelay::Prepare(std::size_t capacity) {
  if (capacity < 2) {
    throw std::invalid_argument("delay capacity must be at least two samples");
  }
  buffer_.assign(capacity, 0.0F);
  writeIndex_ = 0;
}

void PitchTrailsProcessor::CircularDelay::Reset() noexcept {
  std::fill(buffer_.begin(), buffer_.end(), 0.0F);
  writeIndex_ = 0;
}

void PitchTrailsProcessor::CircularDelay::Write(float value) noexcept {
  if (buffer_.empty()) {
    return;
  }
  buffer_[writeIndex_] = value;
  writeIndex_ = (writeIndex_ + 1) % buffer_.size();
}

float PitchTrailsProcessor::CircularDelay::Read(float delaySamples) const noexcept {
  if (buffer_.empty()) {
    return 0.0F;
  }

  const auto maximum = static_cast<float>(buffer_.size() - 1);
  const auto boundedDelay = std::clamp(delaySamples, 1.0F, maximum);
  auto readPosition = static_cast<float>(writeIndex_) - boundedDelay;
  while (readPosition < 0.0F) {
    readPosition += static_cast<float>(buffer_.size());
  }

  const auto indexA = static_cast<std::size_t>(readPosition) % buffer_.size();
  const auto indexB = (indexA + 1) % buffer_.size();
  const auto fraction = readPosition - std::floor(readPosition);
  return buffer_[indexA] + fraction * (buffer_[indexB] - buffer_[indexA]);
}

void PitchTrailsProcessor::AllPass::Prepare(std::size_t delaySamples) {
  buffer_.assign(std::max<std::size_t>(delaySamples, 1), 0.0F);
  index_ = 0;
}

void PitchTrailsProcessor::AllPass::Reset() noexcept {
  std::fill(buffer_.begin(), buffer_.end(), 0.0F);
  index_ = 0;
}

float PitchTrailsProcessor::AllPass::Process(float input, float feedback) noexcept {
  if (buffer_.empty()) {
    return input;
  }
  const auto delayed = buffer_[index_];
  const auto output = delayed - feedback * input;
  buffer_[index_] = input + feedback * output;
  index_ = (index_ + 1) % buffer_.size();
  return output;
}

void PitchTrailsProcessor::Prepare(double sampleRate, double maximumDelaySeconds) {
  if (sampleRate <= 0.0 || maximumDelaySeconds <= 0.0) {
    throw std::invalid_argument("sample rate and maximum delay must be positive");
  }

  sampleRate_ = sampleRate;
  const auto capacity = static_cast<std::size_t>(std::ceil(sampleRate * maximumDelaySeconds)) + 8192U;
  maximumDelaySamples_ = static_cast<float>(capacity - 2U);

  const std::array<double, 2> spread{0.0, 0.0031};
  for (std::size_t channel = 0; channel < channels_.size(); ++channel) {
    channels_[channel].delay.Prepare(capacity);
    channels_[channel].diffusionA.Prepare(static_cast<std::size_t>(sampleRate * (0.0297 + spread[channel])));
    channels_[channel].diffusionB.Prepare(static_cast<std::size_t>(sampleRate * (0.0411 - spread[channel])));
  }
  Reset();
}

void PitchTrailsProcessor::Reset() noexcept {
  phase_ = 0.0F;
  for (auto& channel : channels_) {
    channel.delay.Reset();
    channel.diffusionA.Reset();
    channel.diffusionB.Reset();
    channel.feedbackMemory = 0.0F;
  }
}

void PitchTrailsProcessor::SetParameters(PitchTrailsParameters parameters) noexcept {
  parameters.delayMs = Clamp(parameters.delayMs, 10.0F, 2000.0F);
  parameters.semitones = Clamp(parameters.semitones, -12.0F, 12.0F);
  parameters.feedback = Clamp(parameters.feedback, 0.0F, 0.92F);
  parameters.diffusion = Clamp(parameters.diffusion, 0.0F, 1.0F);
  parameters.mix = Clamp(parameters.mix, 0.0F, 1.0F);
  parameters_ = parameters;
}

std::array<float, 2> PitchTrailsProcessor::ProcessFrame(float left, float right) noexcept {
  const std::array<float, 2> dry{left, right};
  std::array<float, 2> output{};

  for (std::size_t channel = 0; channel < channels_.size(); ++channel) {
    auto& state = channels_[channel];
    const auto delayed = ReadPitched(state.delay, phase_);
    const auto diffusionFeedback = 0.25F + 0.45F * parameters_.diffusion;
    const auto diffuseA = state.diffusionA.Process(delayed, diffusionFeedback);
    const auto diffuseB = state.diffusionB.Process(diffuseA, diffusionFeedback * 0.86F);
    const auto wet = delayed + parameters_.diffusion * (diffuseB - delayed);

    const auto feedbackInput = dry[channel] + state.feedbackMemory * parameters_.feedback;
    state.delay.Write(std::clamp(feedbackInput, -2.0F, 2.0F));
    state.feedbackMemory = std::clamp(wet, -2.0F, 2.0F);
    output[channel] = dry[channel] + parameters_.mix * (wet - dry[channel]);
  }

  const auto ratio = std::pow(2.0F, parameters_.semitones / 12.0F);
  const auto grainSamples = static_cast<float>(sampleRate_ * 0.080);
  phase_ = WrapPhase(phase_ + std::abs(ratio - 1.0F) / std::max(128.0F, grainSamples));
  return output;
}

float PitchTrailsProcessor::Clamp(float value, float minimum, float maximum) noexcept {
  return std::max(minimum, std::min(maximum, value));
}

float PitchTrailsProcessor::ReadPitched(const CircularDelay& delay, float phase) const noexcept {
  const auto baseDelay = std::min(parameters_.delayMs * static_cast<float>(sampleRate_) / 1000.0F,
                                  maximumDelaySamples_ - 4096.0F);
  const auto ratio = std::pow(2.0F, parameters_.semitones / 12.0F);
  if (std::abs(ratio - 1.0F) < 1.0e-4F) {
    return delay.Read(baseDelay);
  }

  const auto grainSamples = std::min(static_cast<float>(sampleRate_ * 0.080), maximumDelaySamples_ - baseDelay);
  const auto phaseA = WrapPhase(phase);
  const auto phaseB = WrapPhase(phase + 0.5F);
  const auto delayForPhase = [&](float grainPhase) {
    return ratio > 1.0F ? baseDelay + (1.0F - grainPhase) * grainSamples
                        : baseDelay + grainPhase * grainSamples;
  };

  const auto weightA = Hann(phaseA);
  const auto weightB = Hann(phaseB);
  return delay.Read(delayForPhase(phaseA)) * weightA + delay.Read(delayForPhase(phaseB)) * weightB;
}

}  // namespace wfe::dsp
