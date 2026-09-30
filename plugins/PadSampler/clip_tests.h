#pragma once
void TestClipTrim() {
  ClipTrim full;Require(full.Valid(),"full clip is valid");
  Require(ResolveTrim({.01,.02},48000,48000).start==480,"clip seconds convert to source frames");
  Require(ResolveTrim({.01,.02},48000,48000).end==960,"clip end is exclusive");
  Require(ResolveTrim({.5,{}},48000,12000).reset,"too-short relink resets to full source");
  ClipHistory history;ClipTrim before{.1,.4},after{.2,.4};history.Commit(before,after);auto edit=after;
  Require(history.Undo(edit) && edit==before,"clip undo restores start");Require(history.Redo(edit) && edit==after,"clip redo restores end");
  for(int i=0;i<40;++i){auto next=ClipTrim{double(i)/1000.,{}};history.Commit(edit,next);edit=next;}Require(history.nUndo==32,"clip history bounded at 32 edits");
  Json slots=Json::array();for(int i=0;i<6;++i)slots.push_back({{"name","Pad"},{"path",""},{"clip",{{"start",0.},{"end",nullptr}}}});
  Json doc={{"version",3},{"parameters",std::vector<double>(56,0.)},{"slots",slots},{"curves",CurveDocument(CurveExchange::Bank{})}};
  doc["slots"][0]["clip"]={{"start",.1},{"end",.3}};ValidateDocument(doc);
  Require(DocumentTrim(Json::parse(doc.dump()),0)==ClipTrim{.1,.3},"v3 trim round trip");
  auto old=doc;old["version"]=2;for(auto& slot:old["slots"])slot.erase("clip");ValidateDocument(old);Require(DocumentTrim(old,0)==ClipTrim{},"v2 loads full-length clip");
  old["version"]=1;old.erase("curves");ValidateDocument(old);Require(DocumentTrim(old,0)==ClipTrim{},"v1 loads full-length clip");
  for(int bad=0;bad<6;++bad){auto malformed=doc;
    if(bad==0)malformed["slots"][0]["clip"]["start"]=-1;
    if(bad==1)malformed["slots"][0]["clip"]["end"]=.05;
    if(bad==2)malformed["slots"][0]["clip"]["start"]="bad";
    if(bad==3)malformed["slots"][0]["clip"].erase("end");
    if(bad==4)malformed["slots"][0]["clip"]["end"]=61;
    if(bad==5)malformed["slots"][0]["clip"]="bad";
    bool rejected=false;try{ValidateDocument(malformed);}catch(...){rejected=true;}Require(rejected,"malformed v3 trim rejected before restore");
  }
  Sample sample;sample.rate=48000;sample.channels=1;sample.data.resize(500);
  for(int i=0;i<500;++i)sample.data[i]=float(i)/500.f;
  sample.SetTrim(100,300);Engine<IPlugFilter> a,b;a.Reset(48000);b.Reset(48000);a.master=b.master=1.;
  for(auto* e:{&a,&b}){e->settings[0].velocityAmount=0;e->settings[0].bypass=true;e->Publish(0,&sample);}
  double al[256]{},ar[256]{},bl[256]{},br[256]{};
  a.Midi(0,36,1,127);b.Midi(0,36,1,127);a.Process(al,ar,80);b.Process(bl,br,80);
  Require(std::abs(al[0]-(100./500./48.))<.001,"first frame uses trim start and click-free fade");
  Require(std::abs(al[50]-150./500.)<.002,"trim plays exact source frames at unchanged pitch");
  sample.SetTrim(200,300);a.Process(al,ar,80);b.Process(bl,br,80);
  for(int i=0;i<80;++i)Require(al[i]==bl[i],"ringing voices retain their captured trim");
  a.Reset(48000);b.Reset(48000);
  a.Midi(0,36,1,127);b.Midi(0,36,1,127);a.Process(al,ar,80);b.Process(bl,br,80);
  Require(std::abs(al[50]-250./500.)<.002,"new strike uses edited start frame");
  a.Reset(48000);sample.SetTrim(300,302);a.Midi(0,36,1,127);a.Process(al,ar,80);
  for(int i=0;i<80;++i)Require(std::isfinite(al[i]),"two-frame range stays finite");
  Require(al[0]>0 && al[1]>0 && al[2]==0 && a.ActiveVoices()==0,"two-frame clip plays exactly two frames and ends quietly");
  a.Reset(96000);sample.SetTrim(100,300);a.Midi(0,36,1,127);a.Process(al,ar,256);
  Require(std::abs(al[100]-150./500.)<.002,"trim uses source frames across sample-rate conversion");
  inAudio=true;sample.SetTrim(200,400);a.Midi(0,36,1,127);a.Process(al,ar,256);inAudio=false;
  Require(audioAllocations==0,"trim publication and playback allocate nothing in audio callback");
}
