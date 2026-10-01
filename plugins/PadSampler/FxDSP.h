#pragma once
#include "PadFX.h"
#include <array>
#include <cmath>
#include <cstdint>

namespace padsampler {
// One instance per pad. Delay storage is fixed for 1 s at up to 384 kHz.
// Generation tags make Stop All a constant-time reset, even at the largest rate.
class FxDSP {
  template<size_t N> struct Ring {
    std::array<float,N> data{};
    std::array<uint32_t,N> tags{};
    size_t pos=0;
    uint32_t generation=1;
    float Read(size_t distance) const {size_t i=(pos+N-distance%N)%N;return tags[i]==generation?data[i]:0.f;}
    void Write(float value) {data[pos]=value;tags[pos]=generation;if(++pos==N)pos=0;}
    void Clear() {if(++generation==0){tags.fill(0);generation=1;}pos=0;}
  };
  struct StereoRing {
    Ring<384001> l,r;
    void Clear(){l.Clear();r.Clear();}
  } delay_;
  struct SmallRing {Ring<32768> l,r;void Clear(){l.Clear();r.Clear();}} reverb_;
  struct FlangeRing {Ring<8192> l,r;void Clear(){l.Clear();r.Clear();}} flanger_;
  struct ChorusRing {Ring<16384> l,r;void Clear(){l.Clear();r.Clear();}} chorus_;
  // Four-times interpolation and a 31-tap low-pass before decimation keep
  // nonlinear harmonics out of the audible base band.
  struct Shaper {
    std::array<double,31> kernel{};
    std::array<std::array<double,31>,2> history{};
    std::array<double,2> previous{}, dcInput{}, dcOutput{};
    std::array<int,2> position{};
    Shaper(){
      double sum=0.;
      for(int i=0;i<31;++i){double n=i-15.;double sinc=n==0.?0.25:std::sin(3.141592653589793*.25*n)/(3.141592653589793*n);
        double window=.42-.5*std::cos(2.*3.141592653589793*i/30.)+.08*std::cos(4.*3.141592653589793*i/30.);
        kernel[i]=sinc*window;sum+=kernel[i];}
      for(auto& c:kernel)c/=sum;
    }
    void Clear(){for(auto& h:history)h.fill(0.);previous.fill(0.);dcInput.fill(0.);dcOutput.fill(0.);position.fill(0);}
    double Process(double input,int channel,double drive,bool hard){
      double result=0.,start=previous[channel];previous[channel]=input;
      for(int phase=1;phase<=4;++phase){
        double x=(start+(input-start)*phase*.25)*drive;
        double shaped=hard?std::clamp(x+.12*x*x,-.85,.75):std::tanh(x);
        auto& h=history[channel];int& pos=position[channel];h[pos]=shaped;if(++pos==31)pos=0;
      }
      int index=position[channel];
      for(int tap=0;tap<31;++tap){if(--index<0)index=30;result+=kernel[tap]*history[channel][index];}
      // Asymmetric clipping requires a DC blocker; its coefficient stays stable at 8 kHz.
      double out=result-dcInput[channel]+.995*dcOutput[channel];dcInput[channel]=result;dcOutput[channel]=out;
      return out;
    }
  } saturation_,distortion_;
  std::array<double,kFxValuesPerPad> smooth_{};
  bool initialized_=false;
  double rate_=44100.,delayToneL_=0.,delayToneR_=0.,reverbDampL_=0.,reverbDampR_=0.,envelope_=0.,flangePhase_=0.,chorusPhase_=0.,tremoloPhase_=0.;
  std::array<std::array<double,2>,2> nonlinearTone_{};
  FxChain chain_{}, pending_{};
  int topologyFade_=0, stopFade_=0;
  static double Clip(double v){return std::clamp(v,-16.,16.);}
  static double Mix(double dry,double wet,double amount){return dry+(wet-dry)*amount;}
  static double Db(double v){return std::pow(10.,v/20.);}
  template<size_t N> static double ReadLerp(const Ring<N>& ring,double distance){auto n=size_t(distance);double t=distance-n;return ring.Read(n)*(1.-t)+ring.Read(n+1)*t;}
 public:
  void SetRate(double rate){rate_=std::clamp(rate,8000.,384000.);Clear();initialized_=false;}
  void SetChain(const FxChain& chain){if(!chain.Valid() || chain==pending_)return;pending_=chain;topologyFade_=std::max(16,int(rate_*.005));}
  void Stop(){stopFade_=std::max(16,int(rate_*.01));}
  void Clear(){delay_.Clear();reverb_.Clear();flanger_.Clear();chorus_.Clear();saturation_.Clear();distortion_.Clear();for(auto& t:nonlinearTone_)t.fill(0.);delayToneL_=delayToneR_=reverbDampL_=reverbDampR_=envelope_=flangePhase_=chorusPhase_=tremoloPhase_=0.;eqLow_.fill(0.);eqHigh_.fill(0.);eqIc1_.fill(0.);eqIc2_.fill(0.);eqCountdown_=0;}
  bool Active() const{return chain_.count || pending_.count || topologyFade_ || stopFade_;}
  void Process(double& l,double& r,const FxSettings& settings){
    if(!initialized_){smooth_=settings.values;initialized_=true;}
    const double coefficient=std::exp(-1./(rate_*.01));
    for(int i=0;i<kFxValuesPerPad;++i)smooth_[i]=coefficient*smooth_[i]+(1.-coefficient)*settings.values[i];
    if(topologyFade_==1){chain_=pending_;topologyFade_=-std::max(16,int(rate_*.005));}
    const double dryL=l,dryR=r;
    for(int i=0;i<chain_.count;++i){
      FxType type=chain_.order[i];const int o=kFxOffsets[int(type)];
      auto p=[&](int index){return smooth_[o+index];};
      // Bypass targets wet mix; smoothed bypass changes remain click free.
      double amount=std::clamp(p(0),0.,1.)*(1.-std::clamp(smooth_[o+kFxCounts[int(type)]-1],0.,1.));
      double a=l,b=r;
      if(type==FxType::Delay){
        size_t distance=std::clamp<size_t>(size_t(p(1)*rate_*.001),1,384000);
        double dl=delay_.l.Read(distance),dr=delay_.r.Read(distance);
        double alpha=std::clamp(2.*3.141592653589793*p(3)/rate_,0.,1.);
        delayToneL_+=alpha*(dl-delayToneL_);delayToneR_+=alpha*(dr-delayToneR_);
        a=delayToneL_;b=delayToneR_;
        double feedback=std::clamp(p(2),0.,.9);
        delay_.l.Write(float(Clip(l+feedback*a)));delay_.r.Write(float(Clip(r+feedback*b)));
      }else if(type==FxType::Reverb){
        const double size=std::clamp(p(1),0.,1.),damping=std::clamp(p(2),0.,1.);
        const size_t d1=std::clamp<size_t>(size_t(rate_*(.039+.041*size)),1,32767);
        const size_t d2=std::clamp<size_t>(size_t(rate_*(.053+.057*size)),1,32767);
        double wl=.62*reverb_.l.Read(d1)+.38*reverb_.l.Read(d2);
        double wr=.62*reverb_.r.Read(std::min<size_t>(32767,d1+97))+.38*reverb_.r.Read(std::min<size_t>(32767,d2+131));
        reverbDampL_+= (1.-damping*.92)*(wl-reverbDampL_);
        reverbDampR_+= (1.-damping*.92)*(wr-reverbDampR_);
        double feedback=.35+.53*size;
        reverb_.l.Write(float(Clip(l+feedback*reverbDampL_)));
        reverb_.r.Write(float(Clip(r+feedback*reverbDampR_)));
        double width=std::clamp(p(3),0.,1.);
        a=Mix((wl+wr)*.5,wl,width);b=Mix((wl+wr)*.5,wr,width);
      }else if(type==FxType::Compressor){
        double level=std::max(std::abs(l),std::abs(r));
        double tau=(level>envelope_?p(3):p(4))*.001;
        double coeff=std::exp(-1./(rate_*std::max(.0001,tau)));
        envelope_=coeff*envelope_+(1.-coeff)*level;
        double db=20.*std::log10(std::max(envelope_,1.e-9));
        double reduction=db>p(1)?(p(1)-db)*(1.-1./std::max(1.,p(2))):0.;
        double gain=Db(reduction+p(5));a=l*gain;b=r*gain;
      }else if(type==FxType::EQ){
        // Low/high shelves and a true resonant SVF mid band. Coefficients are
        // refreshed at a bounded interval from smoothed host controls.
        const double lowAlpha=std::clamp(2.*3.141592653589793*180./rate_,0.,1.);
        const double highAlpha=std::clamp(2.*3.141592653589793*6000./rate_,0.,1.);
        if(eqCountdown_--<=0){
          double freq=std::clamp(p(4),100.,std::min(10000.,rate_*.45));
          double g=std::tan(3.141592653589793*freq/rate_);
          eqR_=1./std::clamp(p(5),.2,8.);
          eqA1_=1./(1.+g*(g+eqR_));eqA2_=g*eqA1_;eqA3_=g*eqA2_;eqCountdown_=31;
        }
        auto eq=[&](double in,int c){
          eqLow_[c]+=lowAlpha*(in-eqLow_[c]);eqHigh_[c]+=highAlpha*(in-eqHigh_[c]);
          double v3=in-eqIc2_[c],v1=eqA1_*eqIc1_[c]+eqA2_*v3,v2=eqIc2_[c]+eqA2_*eqIc1_[c]+eqA3_*v3;
          eqIc1_[c]=2.*v1-eqIc1_[c];eqIc2_[c]=2.*v2-eqIc2_[c];
          double band=eqR_*v1;
          return Clip(in+(Db(p(1))-1.)*eqLow_[c]+(Db(p(2))-1.)*band+(Db(p(3))-1.)*(in-eqHigh_[c]));
        };
        a=eq(l,0);b=eq(r,1);
      }else if(type==FxType::Flanger){
        double depth=std::clamp(p(2)*rate_*.001,0.,rate_*.01);
        double distance=std::clamp(1.+depth*(1.+std::sin(flangePhase_))*.5,1.,double(8190));
        a=ReadLerp(flanger_.l,distance);b=ReadLerp(flanger_.r,distance);
        double feedback=std::clamp(p(3),-.8,.8);
        flanger_.l.Write(float(Clip(l+a*feedback)));flanger_.r.Write(float(Clip(r+b*feedback)));
        flangePhase_+=2.*3.141592653589793*std::clamp(p(1),.01,10.)/rate_;
        if(flangePhase_>=2.*3.141592653589793)flangePhase_-=2.*3.141592653589793;
      }else if(type==FxType::Chorus){
        const double depth=std::clamp(p(2),0.,10.)*.001*rate_;
        const double offset=std::clamp(p(3),0.,1.)*3.141592653589793;
        auto tap=[&](const Ring<16384>& ring,double phase){
          double d1=rate_*.012+depth*std::sin(phase);
          double d2=rate_*.012+depth*std::sin(phase+1.5707963267948966);
          return .5*(ReadLerp(ring,std::clamp(d1,1.,16381.))+ReadLerp(ring,std::clamp(d2,1.,16381.)));
        };
        a=tap(chorus_.l,chorusPhase_);b=tap(chorus_.r,chorusPhase_+offset);
        chorus_.l.Write(float(Clip(l)));chorus_.r.Write(float(Clip(r)));
        chorusPhase_+=2.*3.141592653589793*std::clamp(p(1),.05,8.)/rate_;
        if(chorusPhase_>=2.*3.141592653589793)chorusPhase_-=2.*3.141592653589793;
      }else if(type==FxType::Saturation || type==FxType::Distortion){
        const bool hard=type==FxType::Distortion;
        auto& shaper=hard?distortion_:saturation_;
        const int index=hard?1:0;
        const double drive=Db(std::clamp(p(1),0.,hard?36.:24.));
        const double alpha=std::clamp(2.*3.141592653589793*std::min(p(2),rate_*.45)/rate_,0.,1.);
        auto shape=[&](double in,int channel){
          const double wet=shaper.Process(in,channel,drive,hard);
          double& tone=nonlinearTone_[index][channel];tone+=alpha*(wet-tone);
          return Clip(tone*Db(std::clamp(p(3),hard?-36.:-24.,6.)));
        };
        a=shape(l,0);b=shape(r,1);
      }else if(type==FxType::Tremolo){
        const double phaseOffset=std::clamp(p(4),0.,180.)*3.141592653589793/180.;
        auto gain=[&](double phase){double sine=std::sin(phase);double rounded=std::tanh(4.*sine)/std::tanh(4.);
          double wave=.5+.5*((1.-std::clamp(p(3),0.,1.))*sine+std::clamp(p(3),0.,1.)*rounded);
          return 1.-std::clamp(p(2),0.,1.)*(1.-wave);};
        a=l*gain(tremoloPhase_);b=r*gain(tremoloPhase_+phaseOffset);
        tremoloPhase_+=2.*3.141592653589793*std::clamp(p(1),.05,20.)/rate_;
        if(tremoloPhase_>=2.*3.141592653589793)tremoloPhase_-=2.*3.141592653589793;
      }
      l=Mix(l,a,amount);r=Mix(r,b,amount);
    }
    if(topologyFade_>0){double gain=double(topologyFade_)/std::max(16,int(rate_*.005));l*=gain;r*=gain;--topologyFade_;}
    else if(topologyFade_<0){double gain=1.-double(-topologyFade_)/std::max(16,int(rate_*.005));l*=gain;r*=gain;++topologyFade_;}
    if(stopFade_>0){double gain=double(stopFade_)/std::max(16,int(rate_*.01));l*=gain;r*=gain;if(--stopFade_==0)Clear();}
    if(!chain_.count && !pending_.count && !topologyFade_ && !stopFade_){l=dryL;r=dryR;}
  }
 private:
  std::array<double,2> eqLow_{},eqHigh_{},eqIc1_{},eqIc2_{};
  double eqA1_=1.,eqA2_=0.,eqA3_=0.,eqR_=1.;
  int eqCountdown_=0;
};
}
