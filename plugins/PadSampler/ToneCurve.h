#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <mutex>

namespace padsampler {
struct CurvePoint { double x = 0., y = 0.; };
struct ToneShape {
  bool legacy = false;
  int count = 2;
  std::array<CurvePoint, 8> points{{{0,0},{1,1}}};
  bool Valid() const {
    if (count < 2 || count > 8 || points[0].x != 0 || points[0].y != 0 || points[count-1].x != 1 || points[count-1].y != 1) return false;
    for (int i=0;i<count;++i) if (!std::isfinite(points[i].x) || !std::isfinite(points[i].y) || points[i].x<0 || points[i].x>1 || points[i].y<0 || points[i].y>1 || (i && (points[i].x<=points[i-1].x || points[i].y<points[i-1].y))) return false;
    return true;
  }
  double Evaluate(double x, double exponent=1.) const {
    x=std::clamp(x,0.,1.);
    if (legacy) return std::pow(x,exponent);
    for(int i=1;i<count;++i) if(x<=points[i].x) {
      const auto a=points[i-1], b=points[i];
      return a.y+(b.y-a.y)*(x-a.x)/(b.x-a.x);
    }
    return 1.;
  }
  static ToneShape Preset(int type, double exponent=1.) {
    ToneShape c; c.count=7;
    constexpr double xs[]={0,.125,.25,.5,.75,.875,1};
    for(int i=0;i<7;++i) { double x=xs[i]; c.points[i]={x, type==1 ? std::sqrt(x) : type==2 ? x*x : type==3 ? 3*x*x-2*x*x*x : type==4 ? std::pow(x,exponent) : x}; }
    return c;
  }
  void Materialize(double exponent) { if(legacy) *this=Preset(4,exponent); }
  bool Move(int index,double x,double y) {
    if(index<=0 || index>=count-1) return false;
    const double lo=std::nextafter(points[index-1].x,1.), hi=std::nextafter(points[index+1].x,0.);
    points[index]={lo<=hi?std::clamp(x,lo,hi):points[index].x,std::clamp(y,points[index-1].y,points[index+1].y)}; return true;
  }
  int Add(double x,double y) {
    if(count==8 || x<=0 || x>=1) return -1;
    int i=1; while(i<count && points[i].x<x) ++i;
    if(x-points[i-1].x<1./12700. || points[i].x-x<1./12700.) return -1;
    for(int j=count;j>i;--j) points[j]=points[j-1];
    ++count; points[i]={x,std::clamp(y,points[i-1].y,points[i+1].y)}; return i;
  }
  bool Remove(int i) { if(i<=0 || i>=count-1) return false; for(int j=i;j<count-1;++j) points[j]=points[j+1]; --count; return true; }
  bool operator==(const ToneShape& b) const {
    if(legacy!=b.legacy || count!=b.count) return false;
    for(int i=0;i<count;++i) if(points[i].x!=b.points[i].x || points[i].y!=b.points[i].y) return false;
    return true;
  }
};
struct CurveHistory {
  std::array<ToneShape,32> undo{}, redo{};
  int nUndo=0,nRedo=0;
  static void Push(std::array<ToneShape,32>& a,int& n,const ToneShape& c) { if(n==32) { std::move(a.begin()+1,a.end(),a.begin()); --n; } a[n++]=c; }
  void Commit(const ToneShape& before,const ToneShape& after) { if(!(before==after)) { Push(undo,nUndo,before); nRedo=0; } }
  bool Undo(ToneShape& c) { if(!nUndo) return false; Push(redo,nRedo,c); c=undo[--nUndo]; return true; }
  bool Redo(ToneShape& c) { if(!nRedo) return false; Push(undo,nUndo,c); c=redo[--nRedo]; return true; }
};
// Atomic fields avoid the data race of an ordinary seqlock. Writers serialize off
// the render thread. The audio reader tries once and retains its previous complete
// snapshot on contention; it never spins, locks, allocates, or observes torn curves.
class CurveExchange {
 public:
  using Bank=std::array<ToneShape,6>;
  CurveExchange() { Store(Bank{}); }
  void Store(const Bank& bank) {
    std::lock_guard<std::mutex> lock(writer_);
    sequence_.fetch_add(1);
    for(int s=0;s<6;++s) {
      counts_[s].store(bank[s].count); legacy_[s].store(bank[s].legacy);
      for(int p=0;p<8;++p) { xy_[s][p*2].store(bank[s].points[p].x); xy_[s][p*2+1].store(bank[s].points[p].y); }
    }
    sequence_.fetch_add(1);
  }
  bool Read(Bank& bank) const {
    auto before=sequence_.load(); if(before&1) return false;
    Bank candidate;
    for(int s=0;s<6;++s) {
      candidate[s].count=counts_[s].load(); candidate[s].legacy=legacy_[s].load();
      for(int p=0;p<8;++p) candidate[s].points[p]={xy_[s][p*2].load(),xy_[s][p*2+1].load()};
    }
    if(sequence_.load()!=before) return false;
    bank=candidate; return true;
  }
  Bank Snapshot() const { Bank bank; while(!Read(bank)) {} return bank; } // Non-audio serialization only.
 private:
  static_assert(std::atomic<double>::is_always_lock_free && std::atomic<unsigned>::is_always_lock_free,"Curve publication requires lock-free atomics");
  mutable std::mutex writer_;
  std::atomic<unsigned> sequence_{0};
  std::array<std::atomic<int>,6> counts_{};
  std::array<std::atomic<bool>,6> legacy_{};
  std::array<std::array<std::atomic<double>,16>,6> xy_{};
};
}
