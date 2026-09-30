#pragma once
#include <cassert>
#include "IPlugUtilities.h"
#include "SVF.h"
namespace padsampler {
struct IPlugFilter {
  iplug::SVF<double, 2> filter;
  void Prepare(double rate, double hz) {
    filter.Reset(); filter.SetSampleRate(rate); filter.SetQ(.7071067811865476); filter.SetFreqCPS(hz);
  }
  void Process(double& l, double& r) { double* channels[] = {&l, &r}; filter.ProcessBlock(channels, channels, 2, 1); }
};
}
