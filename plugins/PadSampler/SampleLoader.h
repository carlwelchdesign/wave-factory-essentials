#pragma once
#include "SamplerEngine.h"
#include <memory>
#include <string>
namespace padsampler {
class SampleLoader {
 public:
  virtual ~SampleLoader() = default;
  virtual std::unique_ptr<Sample> Load(const std::string& path, size_t available) = 0;
  virtual void WriteWave(const std::string& path, const Sample& sample) = 0;
};
std::unique_ptr<SampleLoader> MakeAppleSampleLoader();
}
