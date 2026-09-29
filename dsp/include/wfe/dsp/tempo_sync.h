#pragma once

#include <algorithm>

namespace wfe::dsp {

enum class DelayDivision {
  Sixteenth = 0,
  EighthTriplet,
  Eighth,
  DottedEighth,
  QuarterTriplet,
  Quarter,
  DottedQuarter,
  Half,
  Bar,
};

[[nodiscard]] constexpr double DelayDivisionBeats(DelayDivision division) noexcept {
  switch (division) {
    case DelayDivision::Sixteenth:
      return 0.25;
    case DelayDivision::EighthTriplet:
      return 1.0 / 3.0;
    case DelayDivision::Eighth:
      return 0.5;
    case DelayDivision::DottedEighth:
      return 0.75;
    case DelayDivision::QuarterTriplet:
      return 2.0 / 3.0;
    case DelayDivision::Quarter:
      return 1.0;
    case DelayDivision::DottedQuarter:
      return 1.5;
    case DelayDivision::Half:
      return 2.0;
    case DelayDivision::Bar:
    default:
      return 4.0;
  }
}

[[nodiscard]] inline float DelayMilliseconds(double tempo, DelayDivision division,
                                             int timeSigNumerator = 4,
                                             int timeSigDenominator = 4) noexcept {
  const auto safeTempo = tempo > 1.0 ? tempo : 120.0;
  auto beats = DelayDivisionBeats(division);
  if (division == DelayDivision::Bar && timeSigNumerator > 0 && timeSigDenominator > 0) {
    beats = static_cast<double>(timeSigNumerator) * 4.0 /
            static_cast<double>(timeSigDenominator);
  }
  return static_cast<float>(60000.0 * beats / safeTempo);
}

}  // namespace wfe::dsp
