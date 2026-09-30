#include "IPlugFilter.h"
#include "SamplerEngine.h"
#include "SampleLibrary.h"
#include <iostream>
#include <AudioToolbox/AudioToolbox.h>
#include <cstdlib>
#include <new>
static thread_local bool inAudio = false;
static thread_local size_t audioAllocations = 0;
void* operator new(std::size_t size) { if (inAudio) ++audioAllocations; if (void* p = std::malloc(size ? size : 1)) return p; throw std::bad_alloc(); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void* p) noexcept { ::operator delete(p); }

#include <stdexcept>
#include <chrono>
#include <thread>
using namespace padsampler;
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void Fill(Sample& s, double hz = 8000., int frames = 12000) {
  s.rate = 48000; s.channels = 1; s.data.resize(frames);
  for (int i = 0; i < frames; ++i) s.data[i] = float(.5 * std::sin(6.283185307179586 * hz * i / s.rate));
}
double Energy(int velocity, bool bypass) {
  Sample sample; Fill(sample); Engine<IPlugFilter> e; e.Reset(48000);
  e.settings[0].velocityAmount = 0.; e.settings[0].bypass = bypass;
  e.Publish(0, &sample); e.Midi(0, 36, 1, velocity);
  double l[4096]{}, r[4096]{}; e.Process(l, r, 4096);
  double energy = 0; for (int i = 256; i < 4096; ++i) energy += l[i] * l[i]; return energy;
}
#include "curve_tests.h"
int main() {
  try {
    TestToneCurves();
    Settings settings;
    double previous = 0;
    for (int v = 1; v <= 127; ++v) { double f = Cutoff(settings, v, 22050); Require(f >= previous && f <= 22050 * .45 + .001, "cutoff must increase and remain below Nyquist"); previous = f; }
    Require(Energy(127, false) > Energy(1, false) * 20., "hard hit must contain more high-frequency energy");
    Require(std::abs(Energy(1, true) - Energy(127, true)) < 1e-8, "tone bypass must preserve velocity-independent level");
    Sample first, second; Fill(first); Fill(second, 200.);
    {
      Engine<IPlugFilter> e; e.Reset(48000); e.Publish(0, &first);
      double l[512]{}, r[512]{};
      e.Midi(100, 36, 1, 127); e.Process(l,r,512);
      for(int i=0;i<100;++i) Require(l[i]==0., "MIDI sample offset must be respected");
      Require(e.ActiveVoices()==1, "voice must start");
      e.Publish(0,&second); e.Process(l,r,512);
      Require(first.references.load()==1, "old sample must remain alive for its ringing voice");
      inAudio = true;
      for(int i=0;i<100;++i) e.Midi(0,36,1,127);
      e.Process(l,r,512);
      inAudio = false;
      Require(audioAllocations == 0, "MIDI, voice stealing and audio rendering must not allocate"); Require(e.ActiveVoices()==64, "voice pool must be bounded at 64");
      for(auto x:l) Require(std::isfinite(x), "rapid rolls must stay finite");
      e.stop.store(true); e.Process(l,r,512); Require(e.ActiveVoices()==0, "Stop All must finish fades");
      Require(std::abs(l[511])<1e-12, "Stop All must reach silence");
      e.Midi(600,36,1,127); e.Process(l,r,512); Require(e.ActiveVoices()==0, "future event must wait");
      e.Process(l,r,512); Require(e.ActiveVoices()==1, "future event must survive to next block");
      e.Reset(96000); Require(e.ActiveVoices()==0, "sample-rate reset must release voices");
    }
    Require(first.references.load()==0 && second.references.load()==0,"all asset references must drain");
    {
      Engine<IPlugFilter> e; e.Reset(44100.);
      for(int i=0;i<6;++i) { e.Publish(i,&first); e.settings[i].note=40+i; e.settings[i].channel=2; }
      double l[256]{},r[256]{};
      e.Midi(0,40,1,127); e.Midi(0,40,2,0); e.Process(l,r,256);
      Require(e.ActiveVoices()==0,"channel mismatch and zero-velocity note-on must not trigger");
      for(int i=0;i<6;++i) e.Midi(0,40+i,2,100);
      e.Process(l,r,256); Require(e.ActiveVoices()==6,"six simultaneous mapped sources must play independently");
      e.learn.store(5); e.Midi(0,87,12,74);
      Require(e.learn.load()==-1 && (e.learned.load()&127)==87,"MIDI learn captures the next positive-velocity hit");
      for(int i=0;i<600;++i) e.Midi(0,40,2,127);
      Require(e.droppedEvents.load()>0,"event overflow must be bounded and reported");
    }
    auto loader = MakeAppleSampleLoader();
    auto directory = std::filesystem::temp_directory_path() / ("padsampler-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    loader->WriteWave((directory/"source.wav").string(), first);
    auto decoded = loader->Load((directory/"source.wav").string(), kMemoryLimit);
    Require(decoded->data == first.data, "float WAV round trip preserves sample levels");
    Sample stereo; stereo.rate=44100.; stereo.channels=2; stereo.data={.1f,-.2f,.3f,-.4f};
    loader->WriteWave((directory/"stereo.wav").string(),stereo);
    auto decodedStereo=loader->Load((directory/"stereo.wav").string(),kMemoryLimit);
    Require(decodedStereo->channels==2 && decodedStereo->data==stereo.data,"stereo channels and sample rate must survive decoding");
    {
      std::string path=(directory/"source.aiff").string();
      CFURLRef url=CFURLCreateFromFileSystemRepresentation(nullptr,reinterpret_cast<const UInt8*>(path.data()),path.size(),false);
      AudioStreamBasicDescription format{}; format.mSampleRate=first.rate; format.mFormatID=kAudioFormatLinearPCM;
      format.mFormatFlags=kAudioFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked|kAudioFormatFlagIsBigEndian;
      format.mBytesPerPacket=format.mBytesPerFrame=2; format.mFramesPerPacket=1; format.mChannelsPerFrame=1; format.mBitsPerChannel=16;
      ExtAudioFileRef file=nullptr;
      Require(ExtAudioFileCreateWithURL(url,kAudioFileAIFFType,&format,nullptr,0,&file)==noErr,"create AIFF fixture"); CFRelease(url);
      format.mFormatFlags=kAudioFormatFlagIsFloat|kAudioFormatFlagIsPacked; format.mBytesPerFrame=format.mBytesPerPacket=4; format.mBitsPerChannel=32;
      Require(ExtAudioFileSetProperty(file,kExtAudioFileProperty_ClientDataFormat,sizeof(format),&format)==noErr,"configure AIFF fixture");
      AudioBufferList b{}; b.mNumberBuffers=1; b.mBuffers[0]={1,UInt32(first.data.size()*4),first.data.data()};
      Require(ExtAudioFileWrite(file,UInt32(first.Frames()),&b)==noErr,"write AIFF fixture"); ExtAudioFileDispose(file);
      auto aiff=loader->Load(path,kMemoryLimit); Require(aiff->Frames()==first.Frames(),"AIFF frame count");
      for(size_t i=0;i<first.data.size();++i) Require(std::abs(aiff->data[i]-first.data[i])<.0001,"AIFF PCM must decode at original level");
    }
    bool rejected = false; try { loader->Load((directory/"source.wav").string(), 4); } catch (...) { rejected = true; } Require(rejected,"memory limit must reject before allocation");
    rejected = false; try { loader->Load((directory/"missing.wav").string(), kMemoryLimit); } catch (...) { rejected = true; } Require(rejected,"missing sample must be rejected");
    { std::ofstream corrupt(directory/"corrupt.wav"); corrupt << "not audio"; }
    rejected=false; try { loader->Load((directory/"corrupt.wav").string(),kMemoryLimit); } catch (...) { rejected=true; } Require(rejected,"corrupt audio must fail cleanly");
    Sample longSample; longSample.rate=8000.; longSample.channels=1; longSample.data.resize(480001);
    loader->WriteWave((directory/"long.wav").string(),longSample);
    rejected=false; try { loader->Load((directory/"long.wav").string(),kMemoryLimit); } catch (...) { rejected=true; } Require(rejected,"samples over 60 seconds must be rejected");
    {
      Engine<IPlugFilter> e;
      SampleLibrary library([&](int slot, Sample* s) { return e.Publish(slot,s); }, [&] { e.Shutdown(); });
      auto wait = [&](auto predicate) { for(int i=0;i<500;++i) { double l[64]{},r[64]{}; e.Process(l,r,64); if(predicate()) return; std::this_thread::sleep_for(std::chrono::milliseconds(5)); } throw std::runtime_error("library operation timed out"); };
      library.Load(0,(directory/"source.wav").string());
      wait([&] { return library.View().slots[0].status.find("Ready") == 0; });
      library.Load(0,(directory/"missing.wav").string());
      wait([&] { return library.View().slots[0].status.find("Cannot open") == 0; });
      Require(library.View().slots[0].path == (directory/"source.wav").string(),"failed replacement preserves previous sample");
      std::vector<double> params(kParameterCount,0.);
      CurveExchange::Bank kitCurves; kitCurves[4]=ToneShape::Preset(3);
      library.SaveKit((directory/"portable.padkit").string(),params,kitCurves);
      wait([&] { return std::filesystem::exists(directory/"portable.padkit/kit.json"); });
      std::filesystem::rename(directory/"portable.padkit", directory/"moved.padkit");
      std::filesystem::remove(directory/"source.wav");
      library.OpenKit((directory/"moved.padkit/kit.json").string());
      Json restored; wait([&] { return library.TakeReady(restored); });
      Require(library.View().slots[0].path == (directory/"moved.padkit/Samples/slot-1.wav").string(),"moved kit resolves relative sample paths");
      Require(restored["parameters"].size()==kParameterCount,"kit restores all parameters");
      Require(DocumentCurves(restored)[4]==kitCurves[4],"moved kit preserves custom tone configuration");
      library.Stop(); e.Shutdown();
    }
    std::filesystem::remove_all(directory);
    std::cout << "PadSampler DSP, asset lifetime, import, memory, and portable-kit tests passed\n";
    return 0;
  } catch(const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
