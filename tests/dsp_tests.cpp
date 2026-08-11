#include "wfe/dsp/goodband_processor.h"
#include "wfe/dsp/pitch_trails_processor.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void Expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

void TestGoodbandIdentityAtZeroAmount() {
  wfe::dsp::GoodbandProcessor processor;
  processor.Prepare(48000.0);
  processor.SetParameters({0.0F, wfe::dsp::GoodbandCharacter::Clean, 1.0F, 0.0F});

  for (int index = 0; index < 2048; ++index) {
    const auto left = std::sin(static_cast<float>(index) * 0.013F) * 0.7F;
    const auto right = std::cos(static_cast<float>(index) * 0.017F) * 0.6F;
    const auto output = processor.ProcessFrame(left, right);
    Expect(std::abs(output[0] - left) < 1.0e-6F, "Goodband must be left-channel transparent at zero amount");
    Expect(std::abs(output[1] - right) < 1.0e-6F, "Goodband must be right-channel transparent at zero amount");
  }
}

void TestGoodbandProcessesAndStaysFinite() {
  wfe::dsp::GoodbandProcessor processor;
  processor.Prepare(44100.0);
  processor.SetParameters({0.8F, wfe::dsp::GoodbandCharacter::Warm, 1.0F, -1.0F});

  float totalDifference = 0.0F;
  for (int index = 0; index < 8192; ++index) {
    const auto input = std::sin(static_cast<float>(index) * 0.07F) * 0.9F;
    const auto output = processor.ProcessFrame(input, input * 0.8F);
    Expect(std::isfinite(output[0]) && std::isfinite(output[1]), "Goodband output must stay finite");
    totalDifference += std::abs(output[0] - input);
  }
  Expect(totalDifference > 1.0F, "Goodband amount must produce an audible signal change");
}

void TestPitchTrailsDryIdentity() {
  wfe::dsp::PitchTrailsProcessor processor;
  processor.Prepare(48000.0);
  processor.SetParameters({120.0F, 7.0F, 0.5F, 0.7F, 0.0F});

  for (int index = 0; index < 4096; ++index) {
    const auto left = index == 0 ? 1.0F : 0.0F;
    const auto right = index == 4 ? -0.5F : 0.0F;
    const auto output = processor.ProcessFrame(left, right);
    Expect(output[0] == left && output[1] == right, "Pitch Trails mix at zero must preserve dry audio");
  }
}

void TestPitchTrailsImpulseDelay() {
  constexpr double sampleRate = 48000.0;
  constexpr int expectedDelay = 480;

  wfe::dsp::PitchTrailsProcessor processor;
  processor.Prepare(sampleRate);
  processor.SetParameters({10.0F, 0.0F, 0.0F, 0.0F, 1.0F});

  float peak = 0.0F;
  int peakIndex = -1;
  for (int index = 0; index < 1000; ++index) {
    const auto input = index == 0 ? 1.0F : 0.0F;
    const auto output = processor.ProcessFrame(input, input);
    if (std::abs(output[0]) > peak) {
      peak = std::abs(output[0]);
      peakIndex = index;
    }
  }

  Expect(peak > 0.95F, "Pitch Trails wet delay must preserve an unshifted impulse");
  Expect(std::abs(peakIndex - expectedDelay) <= 1, "Pitch Trails delay time must map to samples");
}

void TestPitchTrailsShiftedFeedbackStaysBounded() {
  wfe::dsp::PitchTrailsProcessor processor;
  processor.Prepare(96000.0);
  processor.SetParameters({35.0F, 12.0F, 0.92F, 1.0F, 1.0F});

  float maximum = 0.0F;
  for (int index = 0; index < 96000; ++index) {
    const auto input = index == 0 ? 1.0F : 0.0F;
    const auto output = processor.ProcessFrame(input, -input);
    Expect(std::isfinite(output[0]) && std::isfinite(output[1]), "Pitch Trails output must stay finite");
    maximum = std::max(maximum, std::max(std::abs(output[0]), std::abs(output[1])));
  }
  Expect(maximum <= 2.01F, "Pitch Trails feedback path must remain bounded");
}

}  // namespace

int main() {
  TestGoodbandIdentityAtZeroAmount();
  TestGoodbandProcessesAndStaysFinite();
  TestPitchTrailsDryIdentity();
  TestPitchTrailsImpulseDelay();
  TestPitchTrailsShiftedFeedbackStaysBounded();
  std::cout << "All plugin DSP tests passed\n";
  return EXIT_SUCCESS;
}
