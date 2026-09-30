#pragma once
static FxSettings DefaultFX() {
  FxSettings settings;
  settings.values={.2,.55,.5,1.,0., .2,250,.35,8000,0., 1.,-18,4,10,100,0.,0., 1.,0,0,0,1000,1,0., .25,.25,3,.25,0.};
  return settings;
}
static std::vector<double> DefaultFXDocumentParameters(){std::vector<double> result(kParameterCount,0.);auto defaults=DefaultFX();for(int pad=0;pad<kSlots;++pad){const double base[]={0,0,1,1500,18000,1,0,double(36+pad),0};for(int i=0;i<kSlotParams;++i)result[pad*kSlotParams+i]=base[i];for(int i=0;i<kFxPerPad;++i)result[kFxFirstParam+pad*kFxPerPad+i]=defaults.values[i];}result[54]=100;result[55]=-6;return result;}
static void TestFX() {
  static_assert(kFxFirstParam==56 && kFxFirstParam+kSlots*kFxPerPad==230);
  FxChain chain;Require(chain.Add(FxType::Delay),"add delay");Require(chain.Add(FxType::Reverb),"add reverb");
  Require(!chain.Add(FxType::Delay),"duplicate FX must be rejected");Require(chain.Move(1,-1) && chain.order[0]==FxType::Reverb,"rack order changes");
  Require(chain.Remove(0) && chain.count==1,"rack removal");
  FxChainExchange exchange;FxChainExchange::Bank bank;bank[2]=chain;exchange.Store(bank);FxChainExchange::Bank read;Require(exchange.Read(read) && read[2]==chain,"complete FX topology publication");
  LibraryView view;for(int i=0;i<kSlots;++i)view.slots[i].name="Pad "+std::to_string(i+1);
  auto doc=SampleLibrary::ComposeDocument(view,DefaultFXDocumentParameters(),CurveExchange::Bank{},bank);
  ValidateDocument(doc);Require(doc["version"]==4 && DocumentFX(doc)[2]==chain,"v4 FX round trip");
  for(const auto& mutation:{"duplicate","unknown","too many"}){
    auto bad=doc;
    if(std::string(mutation)=="duplicate")bad["slots"][2]["effects"]={"Delay","Delay"};
    if(std::string(mutation)=="unknown")bad["slots"][2]["effects"]={"Distortion"};
    if(std::string(mutation)=="too many")bad["slots"][2]["effects"]={"Delay","Reverb","EQ","Flanger"};
    bool rejected=false;try{ValidateDocument(bad);}catch(...){rejected=true;}Require(rejected,"malformed FX topology must be rejected");
  }
  {auto bad=doc;bad["parameters"][FxParam(2,FxType::Delay,2)]=1.5;bool rejected=false;try{ValidateDocument(bad);}catch(...){rejected=true;}Require(rejected,"out-of-range FX feedback must reject the entire document");}
  {auto bad=doc;bad["parameters"][3]=0;bool rejected=false;try{ValidateDocument(bad);}catch(...){rejected=true;}Require(rejected,"v4 must reject an out-of-range legacy pad parameter");}
  for(int version=1;version<=3;++version){auto old=doc;old["version"]=version;old["parameters"]=std::vector<double>(56,0.);if(version==1)old.erase("curves");if(version<3)for(auto& slot:old["slots"])slot.erase("clip");ValidateDocument(old);for(auto& rack:DocumentFX(old))Require(rack.count==0,"old kit must load empty FX rack");}
  auto settings=DefaultFX();
  settings.values[kFxOffsets[int(FxType::EQ)]+1]=6.;
  for(int t=0;t<kFxTypes;++t){
    auto dry=std::make_unique<FxDSP>(),wet=std::make_unique<FxDSP>();dry->SetRate(48000);wet->SetRate(48000);FxChain only;only.Add(FxType(t));wet->SetChain(only);
    double difference=0,tail=0;
    for(int f=0;f<48000;++f){double input=f>=256 && f<10000?std::sin(f*.13)*.3:0.;double a=input,b=input,c=input,d=input;
      dry->Process(a,b,settings);wet->Process(c,d,settings);
      Require(std::isfinite(c) && std::isfinite(d),"FX output must stay finite");
      if(f>=1000 && f<10000)difference+=std::abs(c-a)+std::abs(d-b);
      if(f>=11000)tail+=std::abs(c)+std::abs(d);
    }
    Require(difference>.001,"each FX must alter an audible signal");
    if(t==int(FxType::Reverb) || t==int(FxType::Delay))Require(tail>.001,"time FX must ring after source ends");
  }
  {
    auto a=std::make_unique<FxDSP>(),b=std::make_unique<FxDSP>();a->SetRate(48000);b->SetRate(48000);FxChain first,second;first.Add(FxType::Compressor);first.Add(FxType::Delay);second.Add(FxType::Delay);second.Add(FxType::Compressor);a->SetChain(first);b->SetChain(second);
    auto controls=DefaultFX();controls.values[kFxOffsets[int(FxType::Delay)]]=1.;controls.values[kFxOffsets[int(FxType::Delay)]+1]=10.;controls.values[kFxOffsets[int(FxType::Compressor)]+1]=-40.;
    double difference=0;
    for(int f=0;f<3000;++f){double input=f>=300 && f<650?1.:0.,al=input,ar=input,bl=input,br=input;a->Process(al,ar,controls);b->Process(bl,br,controls);if(f>650)difference+=std::abs(al-bl)+std::abs(ar-br);}
    Require(difference>.01,"effect ordering must change the processed signal");
    controls.values[kFxOffsets[int(FxType::Delay)]+4]=1.;double energy=0;
    for(int f=0;f<1500;++f){double l=0.,r=0.;b->Process(l,r,controls);if(f>600)energy+=std::abs(l)+std::abs(r);}
    Require(energy<.1,"bypass must fade away delay tails");
  }
  {
    Sample source;source.rate=48000;source.channels=1;source.data.resize(100);source.data[20]=1.f;
    Engine<IPlugFilter> engine;engine.Reset(48000);engine.settings[0].bypass=true;engine.settings[0].velocityAmount=0.;engine.fxSettings[0]=DefaultFX();engine.fxSettings[0].values[kFxOffsets[int(FxType::Delay)]]=1.;engine.fxSettings[0].values[kFxOffsets[int(FxType::Delay)]+1]=10.;engine.fxChains[0].Add(FxType::Delay);
    double warmL[256]{},warmR[256]{};engine.Process(warmL,warmR,256);engine.Publish(0,&source);engine.Midi(0,36,1,127);
    double left[2048]{},right[2048]{};
    inAudio=true;engine.Process(left,right,2048);inAudio=false;
    Require(audioAllocations==0,"active FX processing must remain allocation-free");
    double tail=0;for(int i=150;i<2048;++i)tail+=std::abs(left[i]);Require(tail>.001,"pad bus delay must outlive its trimmed source");
    engine.stop.store(true);double stopL[1024]{},stopR[1024]{};engine.Process(stopL,stopR,1024);for(int i=600;i<1024;++i)Require(std::abs(stopL[i])<1e-9,"Stop All must silence FX tails");
  }
  {
    Sample source;Fill(source,440.,4000);
    Engine<IPlugFilter> dry,wet;dry.Reset(48000);wet.Reset(48000);
    for(auto* engine:{&dry,&wet}){engine->settings[1].note=37;engine->settings[1].bypass=true;engine->settings[1].velocityAmount=0.;engine->Publish(1,&source);}
    wet.fxSettings[0]=DefaultFX();wet.fxChains[0].Add(FxType::Reverb);
    double dl[512]{},dr[512]{},wl[512]{},wr[512]{};dry.Process(dl,dr,512);wet.Process(wl,wr,512);
    dry.Midi(0,37,1,127);wet.Midi(0,37,1,127);dry.Process(dl,dr,512);wet.Process(wl,wr,512);
    for(int i=0;i<512;++i)Require(dl[i]==wl[i] && dr[i]==wr[i],"FX on another pad must not alter dry pad output");
  }
  {
    auto fx=std::make_unique<FxDSP>();fx->SetRate(48000);FxChain only;only.Add(FxType::Delay);fx->SetChain(only);settings.values[kFxOffsets[int(FxType::Delay)]+1]=10;
    for(int f=0;f<3000;++f){double l=f==300?1.:0.,r=l;fx->Process(l,r,settings);}
    fx->Stop();double after=0.;for(int f=0;f<1000;++f){double l=0.,r=0.;fx->Process(l,r,settings);Require(std::isfinite(l) && std::isfinite(r),"Stop All must keep FX finite");if(f>600)after=std::max(after,std::abs(l)+std::abs(r));}
    Require(after<1e-9,"Stop All must clear delayed audio after its fade");
  }
}
