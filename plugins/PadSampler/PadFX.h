#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <utility>

namespace padsampler {
enum class FxType : uint8_t { Reverb, Delay, Compressor, EQ, Flanger, Chorus, Saturation, Distortion, Tremolo, Count };
// The original 29-parameter stride is part of the host automation contract.
constexpr int kFxTypes = 9, kFxPerPad = 29, kNewFxPerPad = 21;
constexpr int kFxValuesPerPad = kFxPerPad + kNewFxPerPad;
constexpr int kFxFirstParam = 56, kNewFxFirstParam = 230;
constexpr std::array<int, kFxTypes> kFxOffsets{0,5,10,17,24,29,34,39,44};
constexpr std::array<int, kFxTypes> kFxCounts{5,5,7,7,5,5,5,5,6};
constexpr std::array<const char*, kFxTypes> kFxNames{"Reverb","Delay","Compressor","EQ","Flanger","Chorus","Saturation","Distortion","Tremolo"};
constexpr int FxParam(int pad, FxType type, int control) {
  const int offset=kFxOffsets[int(type)];
  return offset<kFxPerPad ? kFxFirstParam+pad*kFxPerPad+offset+control
                          : kNewFxFirstParam+pad*kNewFxPerPad+offset-kFxPerPad+control;
}
constexpr std::array<std::pair<double,double>,kFxPerPad> kFxBounds{{
  {0,1},{0,1},{0,1},{0,1},{0,1},
  {0,1},{1,1000},{0,.9},{200,18000},{0,1},
  {0,1},{-60,0},{1,20},{.1,100},{5,1000},{-12,24},{0,1},
  {0,1},{-18,18},{-18,18},{-18,18},{100,10000},{.2,8},{0,1},
  {0,1},{.01,10},{0,10},{-.8,.8},{0,1}
}};
constexpr std::array<std::pair<double,double>,kNewFxPerPad> kNewFxBounds{{
  {0,1},{.05,8},{0,10},{0,1},{0,1},
  {0,1},{0,24},{1000,18000},{-24,6},{0,1},
  {0,1},{0,36},{500,12000},{-36,6},{0,1},
  {0,1},{.05,20},{0,1},{0,1},{0,180},{0,1}
}};
struct FxChain {
  uint8_t count = 0;
  std::array<FxType,3> order{};
  bool Valid() const {
    if(count>3)return false;
    for(int i=0;i<count;++i) {
      if(int(order[i])>=kFxTypes)return false;
      for(int j=0;j<i;++j)if(order[j]==order[i])return false;
    }
    return true;
  }
  bool Contains(FxType type) const {for(int i=0;i<count;++i)if(order[i]==type)return true;return false;}
  bool Add(FxType type) {if(count==3 || int(type)>=kFxTypes || Contains(type))return false;order[count++]=type;return true;}
  bool Remove(int index) {if(index<0 || index>=count)return false;for(int i=index;i<count-1;++i)order[i]=order[i+1];--count;return true;}
  bool Move(int index,int delta) {int to=index+delta;if(index<0 || index>=count || to<0 || to>=count)return false;std::swap(order[index],order[to]);return true;}
  bool operator==(const FxChain& b) const {if(count!=b.count)return false;for(int i=0;i<count;++i)if(order[i]!=b.order[i])return false;return true;}
};
// Atomic fields and a bounded single attempt keep topology publication off the audio thread.
class FxChainExchange {
 public:
  using Bank=std::array<FxChain,6>;
  void Store(const Bank& bank) {
    std::lock_guard<std::mutex> lock(writer_);sequence_.fetch_add(1,std::memory_order_acq_rel);
    for(int s=0;s<6;++s){count_[s].store(bank[s].count);for(int i=0;i<3;++i)order_[s][i].store(uint8_t(bank[s].order[i]));}
    sequence_.fetch_add(1,std::memory_order_release);
  }
  bool Read(Bank& bank) const {
    auto before=sequence_.load(std::memory_order_acquire);if(before&1)return false;
    Bank candidate;for(int s=0;s<6;++s){candidate[s].count=count_[s].load();for(int i=0;i<3;++i)candidate[s].order[i]=FxType(order_[s][i].load());}
    if(sequence_.load(std::memory_order_acquire)!=before)return false;bank=candidate;return true;
  }
  Bank Snapshot() const {Bank bank;while(!Read(bank)){}return bank;}
 private:
  static_assert(std::atomic<uint8_t>::is_always_lock_free && std::atomic<unsigned>::is_always_lock_free);
  mutable std::mutex writer_;
  std::atomic<unsigned> sequence_{0};
  std::array<std::atomic<uint8_t>,6> count_{};
  std::array<std::array<std::atomic<uint8_t>,3>,6> order_{};
};
struct FxSettings {
  // Indexed by the fixed per-pad parameter bank. Written once per render block.
  std::array<double,kFxValuesPerPad> values{};
  double Get(FxType type,int control) const {return values[kFxOffsets[int(type)]+control];}
};
}
