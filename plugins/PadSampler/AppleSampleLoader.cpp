#include "SampleLoader.h"
#include <AudioToolbox/AudioToolbox.h>
#include <filesystem>
#include <stdexcept>
#include <cctype>
namespace padsampler {
namespace {
void Check(OSStatus status, const char* operation) {
  if (status != noErr) throw std::runtime_error(std::string(operation) + " (Core Audio " + std::to_string(status) + ")");
}
struct File {
  ExtAudioFileRef ref = nullptr;
  ~File() { if (ref) ExtAudioFileDispose(ref); }
};
struct URL {
  CFURLRef ref;
  explicit URL(const std::string& path) : ref(CFURLCreateFromFileSystemRepresentation(nullptr, reinterpret_cast<const UInt8*>(path.data()), path.size(), false)) {
    if (!ref) throw std::runtime_error("Invalid sample path");
  }
  ~URL() { CFRelease(ref); }
};
AudioStreamBasicDescription FloatFormat(double rate, int channels) {
  AudioStreamBasicDescription f{};
  f.mSampleRate = rate; f.mFormatID = kAudioFormatLinearPCM;
  f.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
  f.mBytesPerPacket = f.mBytesPerFrame = UInt32(channels * sizeof(float));
  f.mFramesPerPacket = 1; f.mChannelsPerFrame = channels; f.mBitsPerChannel = 32;
  return f;
}
class AppleLoader final : public SampleLoader {
 public:
  std::unique_ptr<Sample> Load(const std::string& path, size_t available) override {
    auto extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    if (extension != ".wav" && extension != ".aif" && extension != ".aiff") throw std::runtime_error("Choose an uncompressed WAV or AIFF file");
    URL url(path); File file;
    Check(ExtAudioFileOpenURL(url.ref, &file.ref), "Cannot open sample; use Load to relink");
    AudioStreamBasicDescription source{}; UInt32 size = sizeof(source);
    Check(ExtAudioFileGetProperty(file.ref, kExtAudioFileProperty_FileDataFormat, &size, &source), "Cannot read sample format");
    if (source.mFormatID != kAudioFormatLinearPCM || (source.mChannelsPerFrame != 1 && source.mChannelsPerFrame != 2)) throw std::runtime_error("Use mono or stereo uncompressed PCM/float audio");
    SInt64 frames = 0; size = sizeof(frames);
    Check(ExtAudioFileGetProperty(file.ref, kExtAudioFileProperty_FileLengthFrames, &size, &frames), "Cannot read sample length");
    if (!std::isfinite(source.mSampleRate) || source.mSampleRate < 8000 || source.mSampleRate > 384000 || frames < 2 || frames / source.mSampleRate > 60.) throw std::runtime_error("Sample must contain 2+ frames, at most 60 seconds, at 8–384 kHz");
    uint64_t count = uint64_t(frames) * source.mChannelsPerFrame;
    if (count > available / sizeof(float)) throw std::runtime_error("Decoded kit exceeds 256 MiB, including samples still playing; stop playback and retry");
    auto result = std::make_unique<Sample>();
    result->rate = source.mSampleRate; result->channels = source.mChannelsPerFrame;
    result->data.resize(size_t(count));
    auto format = FloatFormat(result->rate, result->channels);
    Check(ExtAudioFileSetProperty(file.ref, kExtAudioFileProperty_ClientDataFormat, sizeof(format), &format), "Cannot decode sample");
    UInt32 offset = 0;
    while (offset < frames) {
      UInt32 requested = UInt32(std::min<SInt64>(16384, frames - offset));
      AudioBufferList buffers{}; buffers.mNumberBuffers = 1;
      buffers.mBuffers[0] = {UInt32(result->channels), requested * format.mBytesPerFrame, result->data.data() + size_t(offset) * result->channels};
      UInt32 read = requested;
      Check(ExtAudioFileRead(file.ref, &read, &buffers), "Sample decoding failed");
      if (read == 0) throw std::runtime_error("Sample ended before its declared length");
      offset += read;
    }
    for (float value : result->data) if (!std::isfinite(value)) throw std::runtime_error("Sample contains non-finite audio values");
    return result;
  }
  void WriteWave(const std::string& path, const Sample& sample) override {
    URL url(path); File file; auto format = FloatFormat(sample.rate, sample.channels);
    Check(ExtAudioFileCreateWithURL(url.ref, kAudioFileWAVEType, &format, nullptr, 0, &file.ref), "Cannot create kit sample");
    Check(ExtAudioFileSetProperty(file.ref, kExtAudioFileProperty_ClientDataFormat, sizeof(format), &format), "Cannot configure kit sample");
    AudioBufferList buffers{}; buffers.mNumberBuffers = 1;
    buffers.mBuffers[0] = {UInt32(sample.channels), UInt32(sample.data.size() * sizeof(float)), const_cast<float*>(sample.data.data())};
    Check(ExtAudioFileWrite(file.ref, UInt32(sample.Frames()), &buffers), "Cannot write kit sample");
  }
};
}
std::unique_ptr<SampleLoader> MakeAppleSampleLoader() { return std::make_unique<AppleLoader>(); }
}
