#include "wfe/dsp/goodband_processor.h"

#include <algorithm>
#include <cmath>

namespace wfe::dsp {
namespace {

constexpr double kPi = 3.14159265358979323846;

struct CharacterSettings {
  float drive[3];
  float makeupDb[3];
  float thresholdDb[3];
  float attackMs;
  float releaseMs;
};

[[nodiscard]] CharacterSettings SettingsFor(GoodbandCharacter character) noexcept {
  switch (character) {
    case GoodbandCharacter::Warm:
      return {{1.65F, 1.45F, 1.20F}, {0.8F, 0.5F, 0.1F}, {-7.0F, -8.0F, -6.0F}, 22.0F, 180.0F};
    case GoodbandCharacter::Punch:
      return {{1.25F, 1.35F, 1.15F}, {0.5F, 0.8F, 0.3F}, {-5.0F, -7.0F, -5.0F}, 35.0F, 110.0F};
    case GoodbandCharacter::Wide:
      return {{1.15F, 1.25F, 1.40F}, {0.2F, 0.5F, 0.9F}, {-5.0F, -6.0F, -6.0F}, 18.0F, 150.0F};
    case GoodbandCharacter::Clean:
    default:
      return {{1.10F, 1.10F, 1.08F}, {0.2F, 0.3F, 0.2F}, {-4.0F, -5.0F, -4.0F}, 28.0F, 140.0F};
  }
}

}  // namespace

void GoodbandProcessor::OnePoleLowpass::Prepare(double sampleRate, double cutoffHz) noexcept {
  const auto safeRate = std::max(1.0, sampleRate);
  const auto safeCutoff = std::clamp(cutoffHz, 1.0, safeRate * 0.45);
  coefficient_ = static_cast<float>(1.0 - std::exp(-2.0 * kPi * safeCutoff / safeRate));
}

void GoodbandProcessor::OnePoleLowpass::Reset() noexcept { state_ = 0.0F; }

float GoodbandProcessor::OnePoleLowpass::Process(float input) noexcept {
  state_ += coefficient_ * (input - state_);
  return state_;
}

void GoodbandProcessor::EnvelopeFollower::Prepare(double sampleRate) noexcept {
  sampleRate_ = std::max(1.0, sampleRate);
  Reset();
}

void GoodbandProcessor::EnvelopeFollower::Reset() noexcept { envelope_ = 0.0F; }

float GoodbandProcessor::EnvelopeFollower::Process(float input, float attackMs, float releaseMs) noexcept {
  const auto target = std::abs(input);
  const auto timeMs = target > envelope_ ? attackMs : releaseMs;
  const auto coefficient = static_cast<float>(std::exp(-1.0 / (0.001 * std::max(0.1F, timeMs) * sampleRate_)));
  envelope_ = coefficient * envelope_ + (1.0F - coefficient) * target;
  return envelope_;
}

void GoodbandProcessor::Prepare(double sampleRate) {
  sampleRate_ = std::max(1.0, sampleRate);
  for (auto& channel : channels_) {
    channel.lowSplit.Prepare(sampleRate_, 180.0);
    channel.midSplit.Prepare(sampleRate_, 2800.0);
  }
  for (auto& envelope : envelopes_) {
    envelope.Prepare(sampleRate_);
  }
  Reset();
}

void GoodbandProcessor::Reset() noexcept {
  for (auto& channel : channels_) {
    channel.lowSplit.Reset();
    channel.midSplit.Reset();
  }
  for (auto& envelope : envelopes_) {
    envelope.Reset();
  }
  gainReductionDb_.fill(0.0F);
  dryEnergy_ = 0.0F;
  processedEnergy_ = 0.0F;
  autoGainCompensationDb_ = 0.0F;
}

void GoodbandProcessor::SetParameters(GoodbandParameters parameters) noexcept {
  parameters.amount = Clamp(parameters.amount, 0.0F, 1.0F);
  parameters.mix = Clamp(parameters.mix, 0.0F, 1.0F);
  parameters.outputTrimDb = Clamp(parameters.outputTrimDb, -12.0F, 12.0F);
  parameters_ = parameters;
}

std::array<float, 2> GoodbandProcessor::ProcessFrame(float left, float right) noexcept {
  const std::array<float, 2> dry{left, right};
  std::array<std::array<float, 3>, 2> bands{};

  for (std::size_t channel = 0; channel < channels_.size(); ++channel) {
    const auto input = dry[channel];
    const auto low = channels_[channel].lowSplit.Process(input);
    const auto aboveLow = input - low;
    const auto mid = channels_[channel].midSplit.Process(aboveLow);
    bands[channel] = {low, mid, aboveLow - mid};
  }

  const auto settings = SettingsFor(parameters_.character);
  std::array<float, 3> gains{};
  for (std::size_t band = 0; band < gains.size(); ++band) {
    const auto linkedLevel = std::max(std::abs(bands[0][band]), std::abs(bands[1][band]));
    const auto envelope = envelopes_[band].Process(linkedLevel, settings.attackMs, settings.releaseMs);
    const auto levelDb = 20.0F * std::log10(std::max(envelope, 1.0e-6F));
    const auto ratio = 1.0F + parameters_.amount * 3.0F;
    const auto overDb = std::max(0.0F, levelDb - settings.thresholdDb[band]);
    const auto reductionDb = -overDb * (1.0F - 1.0F / ratio);
    gainReductionDb_[band] = -reductionDb;
    gains[band] = DbToGain(reductionDb + settings.makeupDb[band] * parameters_.amount);
  }

  std::array<float, 2> wet{};
  for (std::size_t channel = 0; channel < channels_.size(); ++channel) {
    for (std::size_t band = 0; band < gains.size(); ++band) {
      wet[channel] += ProcessBand(bands[channel][band], gains[band], settings.drive[band], parameters_.amount);
    }
  }

  if (parameters_.character == GoodbandCharacter::Wide) {
    const auto mid = 0.5F * (wet[0] + wet[1]);
    const auto side = 0.5F * (wet[0] - wet[1]) * (1.0F + 0.18F * parameters_.amount);
    wet = {mid + side, mid - side};
  }

  const std::array<float, 2> mixed{
      dry[0] + parameters_.mix * (wet[0] - dry[0]),
      dry[1] + parameters_.mix * (wet[1] - dry[1]),
  };

  const auto energyCoefficient = static_cast<float>(std::exp(-1.0 / (sampleRate_ * 0.45)));
  const auto dryEnergy = 0.5F * (dry[0] * dry[0] + dry[1] * dry[1]);
  const auto processedEnergy = 0.5F * (mixed[0] * mixed[0] + mixed[1] * mixed[1]);
  dryEnergy_ = energyCoefficient * dryEnergy_ + (1.0F - energyCoefficient) * dryEnergy;
  processedEnergy_ =
      energyCoefficient * processedEnergy_ + (1.0F - energyCoefficient) * processedEnergy;

  float targetCompensationDb = 0.0F;
  if (parameters_.autoGainMatch && dryEnergy_ > 1.0e-8F && processedEnergy_ > 1.0e-8F) {
    targetCompensationDb = Clamp(10.0F * std::log10(dryEnergy_ / processedEnergy_), -12.0F, 12.0F);
  }
  const auto compensationCoefficient = static_cast<float>(std::exp(-1.0 / (sampleRate_ * 0.30)));
  autoGainCompensationDb_ = compensationCoefficient * autoGainCompensationDb_ +
                            (1.0F - compensationCoefficient) * targetCompensationDb;

  const auto outputGain = DbToGain(parameters_.outputTrimDb + autoGainCompensationDb_);
  return {
      mixed[0] * outputGain,
      mixed[1] * outputGain,
  };
}

const std::array<float, 3>& GoodbandProcessor::GetGainReductionDb() const noexcept {
  return gainReductionDb_;
}

float GoodbandProcessor::GetAutoGainCompensationDb() const noexcept {
  return autoGainCompensationDb_;
}

float GoodbandProcessor::Clamp(float value, float minimum, float maximum) noexcept {
  return std::max(minimum, std::min(maximum, value));
}

float GoodbandProcessor::DbToGain(float decibels) noexcept {
  return std::pow(10.0F, decibels / 20.0F);
}

float GoodbandProcessor::ProcessBand(float input, float gain, float drive, float blend) const noexcept {
  if (blend <= 0.0F) {
    return input;
  }
  const auto normalized = std::tanh(input * drive) / std::max(0.01F, std::tanh(drive));
  const auto processed = normalized * gain;
  return input + blend * (processed - input);
}

}  // namespace wfe::dsp
