#pragma once
void TestToneCurves() {
  for(int type=0;type<4;++type) {
    auto c=ToneShape::Preset(type); Require(c.Valid(),"preset is valid");double previous=-1;
    for(int v=0;v<=127;++v){double y=c.Evaluate(v/127.);Require(y>=previous && y>=0 && y<=1,"preset monotonicity");previous=y;}
    for(int i=0;i<c.count;++i){double x=c.points[i].x, expected=type==1?std::sqrt(x):type==2?x*x:type==3?3*x*x-2*x*x*x:x;Require(std::abs(c.Evaluate(x)-expected)<1e-12,"preset exact control points");}
  }
  ToneShape c; Require(!c.Move(0,.2,.2) && !c.Remove(1),"endpoints immutable");
  for(int i=1;i<=6;++i)Require(c.Add(i/7.,i/7.)==i,"add interior points");
  Require(c.count==8 && c.Add(.2,.2)==-1,"eight-point cap");
  c.Move(3,1,-1);Require(c.Valid() && c.points[3].x<c.points[4].x && c.points[3].y==c.points[2].y,"clamp crossing and flat sections");
  Require(c.Remove(3) && c.Valid(),"remove point");
  ToneShape legacy;legacy.legacy=true;
  for(double exponent:{.25,.5,1.,2.,4.})for(int v=0;v<128;++v)Require(legacy.Evaluate(v/127.,exponent)==std::pow(v/127.,exponent),"legacy exactness");
  auto material=legacy;material.Materialize(2.);Require(!material.legacy && material.count==7,"legacy conversion");
  CurveHistory history;history.Commit(legacy,material);auto current=material;Require(history.Undo(current) && current.legacy,"undo restores legacy mode");Require(history.Redo(current) && current==material,"redo restores points");
  for(int i=0;i<40;++i){auto next=ToneShape::Preset(i%4);history.Commit(current,next);current=next;}Require(history.nUndo==32,"undo bound");
  CurveExchange exchange;CurveExchange::Bank bank;bank[2]=ToneShape::Preset(2);exchange.Store(bank);CurveExchange::Bank received;Require(exchange.Read(received) && received[2]==bank[2] && received[0]==ToneShape{},"per-slot snapshot isolation");
  for(auto& slot:bank)slot=ToneShape::Preset(0);exchange.Store(bank);
  std::atomic<bool> finished{false};std::thread writer([&]{for(int i=0;i<5000;++i){auto shape=ToneShape::Preset(i%4);for(auto& slot:bank)slot=shape;exchange.Store(bank);}finished=true;});
  while(!finished){CurveExchange::Bank snapshot;if(exchange.Read(snapshot))for(const auto& shape:snapshot)Require(shape.Valid() && shape==snapshot[0],"concurrent complete snapshots");}writer.join();
  inAudio=true;exchange.Read(received);for(auto& shape:received)shape.Evaluate(.42);inAudio=false;Require(audioAllocations==0,"curve publication/read/evaluation allocates nothing in audio");
  Json slots=Json::array();for(int i=0;i<6;++i)slots.push_back({{"name","Pad"},{"path",""}});
  Json document={{"version",2},{"parameters",std::vector<double>(56,0.)},{"slots",slots},{"curves",CurveDocument(received)}};
  ValidateDocument(document);auto restored=DocumentCurves(Json::parse(document.dump()));for(int i=0;i<6;++i)Require(restored[i]==received[i],"v2 JSON roundtrip");
  auto old=document;old["version"]=1;old.erase("curves");ValidateDocument(old);for(auto& shape:DocumentCurves(old))Require(shape.legacy,"v1 loads legacy mode");
  for(int bad=0;bad<6;++bad){auto malformed=document;
    if(bad==0)malformed["curves"][0]["points"]={{0,0}};
    if(bad==1)malformed["curves"][0]["points"]={{0,0},{.8,.8},{.5,.9},{1,1}};
    if(bad==2)malformed["curves"][0]["points"]={{0,0},{.3,.8},{.7,.2},{1,1}};
    if(bad==3)malformed["curves"][0]["points"]={{0,.1},{1,1}};
    if(bad==4)malformed["curves"][0]["points"]={{0,0},{.5,"invalid"},{1,1}};
    if(bad==5)malformed["curves"]=Json::array();
    bool rejected=false;try{ValidateDocument(malformed);}catch(...){rejected=true;}Require(rejected,"malformed curve rejected");}
  // Identical ringing voices must remain identical after the next-hit settings change.
  Sample sample;Fill(sample,6000.,6000);Engine<IPlugFilter> a,b;a.Reset(48000);b.Reset(48000);a.Publish(0,&sample);b.Publish(0,&sample);
  double al[128]{},ar[128]{},bl[128]{},br[128]{};a.Midi(0,36,1,50);b.Midi(0,36,1,50);a.Process(al,ar,128);b.Process(bl,br,128);
  a.settings[0].shape=ToneShape::Preset(1);a.settings[0].hardHz=6000;
  a.Process(al,ar,128);b.Process(bl,br,128);for(int i=0;i<128;++i)Require(al[i]==bl[i],"curve edits cannot retune ringing voices");
  a.Midi(0,36,1,50);b.Midi(0,36,1,50);a.Process(al,ar,128);b.Process(bl,br,128);bool differs=false;for(int i=0;i<128;++i)differs|=al[i]!=bl[i];Require(differs,"next strike uses new curve");
}
