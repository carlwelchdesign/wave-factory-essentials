#include "PadSampler.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "../shared/WaveFactoryUI.h"
#include <filesystem>
#if defined(APP_API)
#include "IPlugSWELL.h"
#endif
using namespace iplug;
using namespace igraphics;
using namespace padsampler;
#include "PrecisionUI.h"
PadSampler::PadSampler(const InstanceInfo& info) : iplug::Plugin(info, MakeConfig(kParameterCount, 1)), library_([this](int slot, Sample* sample) { return engine_.Publish(slot, sample); }, [this] { engine_.Shutdown(); }) {
  for (int i = 0; i < kSlots; ++i) {
    int p = i * kSlotParams;
    auto label = [i](const char* s) { return "Slot " + std::to_string(i + 1) + " " + s; };
    GetParam(p + Level)->InitDouble(label("Level").c_str(), 0., -60., 12., .1, "dB");
    GetParam(p + Pan)->InitDouble(label("Pan").c_str(), 0., -1., 1., .01);
    GetParam(p + VelocityAmount)->InitDouble(label("Velocity Volume").c_str(), 1., 0., 1., .01);
    GetParam(p + SoftHz)->InitDouble(label("Soft Brightness").c_str(), 1500., 20., 20000., 1., "Hz", 0, "", IParam::ShapeExp());
    GetParam(p + HardHz)->InitDouble(label("Hard Brightness").c_str(), 18000., 20., 20000., 1., "Hz", 0, "", IParam::ShapeExp());
    GetParam(p + ToneCurve)->InitDouble(label("Tone Curve").c_str(), 1., .25, 4., .01);
    GetParam(p + ToneBypass)->InitBool(label("Tone Bypass").c_str(), false);
    GetParam(p + Note)->InitInt(label("MIDI Note").c_str(), 36 + i, 0, 127);
    GetParam(p + Channel)->InitInt(label("MIDI Channel (0=All)").c_str(), 0, 0, 16);
  }
  GetParam(kAuditionVelocity)->InitInt("Audition Velocity", 100, 1, 127);
  GetParam(kMaster)->InitDouble("Master", -6., -60., 0., .1, "dB");
  view_ = library_.View();
  mMakeGraphicsFunc = [&] { return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT)); };
  mLayoutFunc = [this](IGraphics* g) { BuildUI(g); };
}
PadSampler::~PadSampler() { library_.Stop(); engine_.Shutdown(); }
void PadSampler::OnReset() { engine_.Reset(GetSampleRate()); }
void PadSampler::ProcessMidiMsg(const IMidiMsg& msg) {
  if (msg.StatusMsg() == IMidiMsg::kNoteOn && msg.Velocity() > 0) engine_.Midi(msg.mOffset, msg.NoteNumber(), msg.Channel() + 1, msg.Velocity());
  if (msg.StatusMsg() == IMidiMsg::kControlChange && (msg.ControlChangeIdx() == 120 || msg.ControlChangeIdx() == 123)) engine_.stop.store(true);
}
void PadSampler::ProcessBlock(sample**, sample** outputs, int frames) {
  curves_.Read(audioCurves_);
  for (int i = 0; i < kSlots; ++i) {
    int p = i * kSlotParams; auto& s = engine_.settings[i];
    s.level = GetParam(p + Level)->Value(); s.pan = GetParam(p + Pan)->Value();
    s.velocityAmount = GetParam(p + VelocityAmount)->Value(); s.softHz = GetParam(p + SoftHz)->Value();
    s.shape = audioCurves_[i];
    s.hardHz = GetParam(p + HardHz)->Value(); s.curve = GetParam(p + ToneCurve)->Value();
    s.bypass = GetParam(p + ToneBypass)->Bool(); s.note = GetParam(p + Note)->Int(); s.channel = GetParam(p + Channel)->Int();
  }
  engine_.master = std::pow(10., GetParam(kMaster)->Value() / 20.);
  if (NOutChansConnected() >= 2) engine_.Process(outputs[0], outputs[1], frames);
}
void PadSampler::Select(int slot) {
  selected_ = slot; pointSelection = 1; clipPreview_.reset(); ++clipEpoch_;
  if (!GetUI()) return;
  for (int i = 0; i < kSlotParams; ++i) if (controls_[i]) { controls_[i]->SetParamIdx(slot * kSlotParams + i); controls_[i]->SetValueFromDelegate(GetParam(slot * kSlotParams + i)->GetNormalized()); }
  name_->SetStr(view_.slots[slot].name.c_str()); SyncClipMode(); GetUI()->SetAllControlsDirty();
}
void PadSampler::Audition(int slot) { engine_.auditions[slot].store(GetParam(kAuditionVelocity)->Int()); }
void PadSampler::Load(int slot, const std::string& path) { library_.Load(slot, path); }
void PadSampler::ChooseSample() {
  WDL_String file, folder; int slot = selected_;
  GetUI()->PromptForFile(file, folder, EFileAction::Open, "wav aif aiff", [this, slot](const WDL_String& path, const WDL_String&) { if (path.GetLength()) Load(slot, path.Get()); });
}
void PadSampler::ChooseKit(bool save) {
  WDL_String file, folder;
  GetUI()->PromptForFile(file, folder, save ? EFileAction::Save : EFileAction::Open, save ? "padkit" : "json", [this, save](const WDL_String& path, const WDL_String&) {
    if (!path.GetLength()) return;
    if (save) library_.SaveKit(path.Get(), Parameters(), curves_.Snapshot()); else library_.OpenKit(path.Get());
  });
}
std::vector<double> PadSampler::Parameters() const { std::vector<double> values; for (int i = 0; i < kParameterCount; ++i) values.push_back(GetParam(i)->Value()); return values; }
void PadSampler::ApplyParameters(const Json& doc, bool notifyHost) {
  curves_.Store(DocumentCurves(doc)); curveEpoch_.fetch_add(1); resetHistories_.store(true); ++clipEpoch_; clipPreview_.reset();
  for (int i = 0; i < kParameterCount; ++i) {
    GetParam(i)->Set(doc["parameters"][i].get<double>());
    if (notifyHost) { BeginInformHostOfParamChangeFromUI(i); SendParameterValueFromUI(i, GetParam(i)->GetNormalized()); EndInformHostOfParamChangeFromUI(i); }
  }
}
bool PadSampler::SerializeState(IByteChunk& chunk) const {
  auto doc = library_.Document(Parameters(), curves_.Snapshot()).dump(); int length = int(doc.size());
  chunk.Put(&length); chunk.PutBytes(doc.data(), length); return true;
}
int PadSampler::UnserializeState(const IByteChunk& chunk, int position) {
  int length = 0; position = chunk.Get(&length, position);
  if (position < 0 || length < 0 || length > 65536 || length > chunk.Size() - position) return -1;
  try {
    std::string text(size_t(length), '\0'); int end = chunk.GetBytes(text.data(), length, position);
    auto doc = Json::parse(text); ValidateDocument(doc); ApplyParameters(doc, false); library_.Restore(doc); return end;
  } catch (...) { library_.Message("Cannot restore malformed kit/state; current kit preserved"); return -1; }
}
void PadSampler::OnIdle() {
  if(resetHistories_.exchange(false)) { histories_ = {}; clipHistories_ = {}; pointSelection = 1; }
  Json doc; if (library_.TakeReady(doc)) { ApplyParameters(doc, true); if (GetUI()) Select(selected_); }
  int learned = engine_.learned.exchange(-1);
  if (learned >= 0) {
    int p = ((learned >> 24) & 7) * kSlotParams;
    for (auto item : {std::pair<int,int>{p + Note, learned & 127}, {p + Channel, (learned >> 8) & 31}}) {
      GetParam(item.first)->Set(item.second); BeginInformHostOfParamChangeFromUI(item.first); SendParameterValueFromUI(item.first, GetParam(item.first)->GetNormalized()); EndInformHostOfParamChangeFromUI(item.first);
    }
    Message("MIDI captured: note " + std::to_string(learned & 127));
    if (GetUI()) Select(selected_);
  }
  auto nextView = library_.View();
  for(int i=0;i<kSlots;++i) if(nextView.slots[i].status.rfind("Ready",0)==0 && (nextView.slots[i].generation != view_.slots[i].generation || !view_.slots[i].ready)) {
    clipHistories_[i] = {}; if(i==selected_) {clipPreview_.reset(); ++clipEpoch_;}
  }
  view_ = std::move(nextView);
  for (int i = 0; i < kSlots; ++i) { int hit=engine_.hits[i].exchange(0); if(hit) lastHits_[i]=hit; flashes_[i] = reducedMotion_ ? 0.f : std::max(flashes_[i] * .8f, hit / 127.f); }
  if (!GetUI()) return;
  if(controls_[ToneCurve]) controls_[ToneCurve]->SetDisabled(!Shape().legacy);
  if(GetUI()->GetControlInTextEntry()!=name_) name_->SetStr(view_.slots[selected_].name.c_str());
  int midi = engine_.lastMidi.load();
  if (midi >= 0) midiText_ = "MIDI  ·  note " + std::to_string(midi & 127) + "  ·  channel " + std::to_string((midi >> 8) & 31) + "  ·  velocity " + std::to_string((midi >> 16) & 127);
  monitor_->SetStr(midiText_.c_str());
  std::string mapping = "Select a pad · click its name to rename · each strike plays a one-shot";
  for (int i = 0; i < kSlots; ++i) for (int j = i + 1; j < kSlots; ++j) {
    int a = i * kSlotParams, b = j * kSlotParams;
    if (GetParam(a + Note)->Int() == GetParam(b + Note)->Int() && (!GetParam(a + Channel)->Int() || !GetParam(b + Channel)->Int() || GetParam(a + Channel)->Int() == GetParam(b + Channel)->Int())) mapping = "Duplicate MIDI mapping: " + view_.slots[i].name + " and " + view_.slots[j].name + " will both play";
  }
  int learning = engine_.learn.load();
  if (learning >= 0 && learning < kSlots) mapping = "MIDI LEARN: strike " + view_.slots[learning].name;
  if (GetParam(selected_ * kSlotParams + HardHz)->Value() < GetParam(selected_ * kSlotParams + SoftHz)->Value()) mapping = "Hard brightness is below Soft: effective brightness stays at the Soft setting";
  mapping_->SetStr(mapping.c_str());
  std::string status = view_.slots[selected_].status + "  |  " + view_.message;
  if (engine_.droppedEvents.load()) status += "  |  MIDI event overflow: " + std::to_string(engine_.droppedEvents.load());
  if(messageTicks_>0) { --messageTicks_; status=uiMessage_; }
  status_->SetStr(status.c_str()); GetUI()->SetAllControlsDirty();
}

void PadSampler::SetShape(const ToneShape& shape) {
  if(!shape.Valid()) return;
  auto bank=curves_.Snapshot(); bank[selected_]=shape; curves_.Store(bank);
  if(GetUI()) GetUI()->SetAllControlsDirty();
}
void PadSampler::CommitShape(const ToneShape& before) { auto after=Shape(); histories_[selected_].Commit(before,after); if(!(before==after)) MarkStateChanged(); }
void PadSampler::UndoCurve(bool redo) {auto c=Shape();if(redo?History().Redo(c):History().Undo(c)){SetShape(c);MarkStateChanged();pointSelection=std::min(pointSelection,c.count-2);}}
void PadSampler::CurvePreset(int preset) {auto before=Shape();SetShape(ToneShape::Preset(preset));CommitShape(before);pointSelection=1;}
void PadSampler::SyncClipMode() {
  for(auto* c:toneControls_) c->Hide(clipMode_);
  for(auto* c:clipControls_) c->Hide(!clipMode_);
  if(GetUI()) GetUI()->SetAllControlsDirty();
}
void PadSampler::ToggleClipMode() { clipMode_=!clipMode_; clipPreview_.reset(); ++clipEpoch_; SyncClipMode(); }
bool PadSampler::CommitClip(ClipTrim before) {
  auto after=Clip(); clipPreview_.reset();
  if(before==after)return true;
  if(!library_.SetTrim(selected_,after)) {Message("Clip needs a loaded sample and at least two playable frames");return false;}
  view_.slots[selected_].clip=library_.View().slots[selected_].clip;
  clipHistories_[selected_].Commit(before,view_.slots[selected_].clip);
  MarkStateChanged(); if(GetUI())GetUI()->SetAllControlsDirty();return true;
}
void PadSampler::UndoClip(bool redo) {
  auto clip=Clip(); auto history=ClipEdits(); if(!(redo?history.Redo(clip):history.Undo(clip)))return;
  if(library_.SetTrim(selected_,clip)){ClipEdits()=history;view_.slots[selected_].clip=library_.View().slots[selected_].clip;clipPreview_.reset();MarkStateChanged();if(GetUI())GetUI()->SetAllControlsDirty();}
}
void PadSampler::ResetClip() {auto before=Clip();PreviewClip({});CommitClip(before);}
double PadSampler::ToneHz(double velocity) const {
  padsampler::Settings s;int p=selected_*kSlotParams;
  s.softHz=GetParam(p+SoftHz)->Value();s.hardHz=GetParam(p+HardHz)->Value();s.curve=Exponent();s.shape=Shape();
  const double low=std::clamp(s.softHz,20.,std::max(8000.,GetSampleRate())*.45);
  const double high=std::clamp(std::max(s.softHz,s.hardHz),20.,std::max(8000.,GetSampleRate())*.45);
  return std::exp(std::log(low)+s.shape.Evaluate(velocity/127.,s.curve)*(std::log(high)-std::log(low)));
}
void PadSampler::MarkStateChanged() {
#if defined(VST3_API)
  if(auto* handler=GetComponentHandler()) {
    Steinberg::Vst::IComponentHandler2* extended=nullptr;
    if(handler->queryInterface(Steinberg::Vst::IComponentHandler2::iid,reinterpret_cast<void**>(&extended))==Steinberg::kResultOk) {extended->setDirty(true);extended->release();}
  }
#else
  InformHostOfPresetChange();
#endif
}
void PadSampler::ToggleLearn() {if(engine_.learn.load()==selected_) {engine_.learn.store(-1);Message("MIDI Learn cancelled");}else{engine_.learn.store(selected_);Message("MIDI Learn: strike the selected hardware pad");}}
void PadSampler::StopAll() {engine_.stop.store(true);Message("Stop All: ringing voices faded");}
#if defined(APP_API)
extern HWND gHWND;
#endif
void PadSampler::Settings() {
#if defined(APP_API)
  PostMessage(gHWND,WM_COMMAND,40006,0);
#else
  Message("Plugin audio and MIDI routing are controlled by your host");
#endif
}
void PadSampler::BuildUI(IGraphics* g) {
  using namespace precision;
  controls_.fill(nullptr); focusOrder_.clear();toneControls_.clear();clipControls_.clear();focus_=nullptr;
  g->LoadFont(DEFAULT_FONT,"Arial",ETextStyle::Normal);
  g->EnableMouseOver(true);g->AttachTextEntryControl();g->AttachControl(new Backplate(g->GetBounds(),g->LoadBitmap("precision-satin.png")));
  IVStyle style(true,true,{silver,IColor(255,224,231,240),blue,muted,IColor(255,107,170,238),IColor(90,30,40,55),white,blue,ink},IText(12,ink),IText(12,ink),false,true,true,true,.15f,1,2,.8f);
  auto attach=[&](IControl* c){g->AttachControl(c);focusOrder_.push_back(c);return c;};
  auto button=[&](IRECT r,const char* label,std::function<void()> action){return attach(new Action(r,*this,[label]{return std::string(label);},action));};
  g->AttachControl(new ITextControl(IRECT(22,12,300,41),"PadSampler",IText(26,ink,nullptr,EAlign::Near)));
  g->AttachControl(new ITextControl(IRECT(24,40,300,59),"W A V E   F A C T O R Y   /   P R E C I S I O N",IText(9,muted,nullptr,EAlign::Near)));
  button(IRECT(529,22,632,53),"Open Kit",[this]{ChooseKit(false);});
  button(IRECT(643,22,746,53),"Save Kit",[this]{ChooseKit(true);});
  button(IRECT(757,22,862,53),"Settings",[this]{Settings();});
  button(IRECT(873,22,956,53),"? Help",[this]{help_->Hide(!help_->IsHidden());GetUI()->SetAllControlsDirty();});
  const IRECT rects[]={IRECT(22,86,282,230),IRECT(294,86,554,230),IRECT(22,242,282,486),IRECT(294,242,554,486),IRECT(22,498,282,628),IRECT(294,498,554,628)};
  for(int i=0;i<6;++i)attach(new Pad(rects[i],*this,i));
  name_=static_cast<ITextControl*>(attach(new Name(IRECT(588,81,954,115),*this,[this](const std::string& name){library_.Rename(selected_,name);MarkStateChanged();})));
  button(IRECT(588,120,704,150),"Load / Relink",[this]{ChooseSample();});
  button(IRECT(714,120,830,150),"Clear",[this]{library_.Clear(selected_);MarkStateChanged();});
  attach(new Action(IRECT(840,120,954,150),*this,[this]{return engine_.learn.load()==selected_?"Cancel Learn":"MIDI Learn";},[this]{ToggleLearn();}));
  controls_[Note]=attach(new IVNumberBoxControl(IRECT(588,158,762,210),Note,nullptr,"MIDI note",style,true));
  controls_[Channel]=attach(new IVNumberBoxControl(IRECT(776,158,954,210),Channel,nullptr,"Channel · 0 = All",style,true));
  const char* labels[]={"Level","Pan","Velocity Volume"};
  for(int i=0;i<3;++i)controls_[i]=attach(new Knob(IRECT(587+i*125,219,704+i*125,318),i,labels[i],style,*this));
  controls_[SoftHz]=attach(new Knob(IRECT(588,328,705,420),SoftHz,"Soft brightness",style,*this));
  controls_[HardHz]=attach(new Knob(IRECT(711,328,828,420),HardHz,"Hard brightness",style,*this));
  controls_[ToneBypass]=attach(new IVSwitchControl(IRECT(836,328,954,378),ToneBypass,"Tone bypass",style));
  controls_[ToneCurve]=attach(new IVNumberBoxControl(IRECT(836,381,954,420),ToneCurve,nullptr,"Legacy exponent",style,true));
  toneControls_.push_back(attach(new Presets(IRECT(588,429,704,456),*this)));
  toneControls_.push_back(attach(new Action(IRECT(711,429,767,456),*this,[]{return std::string("Undo");},[this]{UndoCurve(false);},[this]{return History().nUndo>0;})));
  toneControls_.push_back(attach(new Action(IRECT(774,429,830,456),*this,[]{return std::string("Redo");},[this]{UndoCurve(true);},[this]{return History().nRedo>0;})));
  clipControls_.push_back(attach(new Action(IRECT(588,429,704,456),*this,[]{return std::string("Reset Length");},[this]{ResetClip();},[this]{return view_.slots[selected_].ready && !(Clip()==padsampler::ClipTrim{});} )));
  clipControls_.push_back(attach(new Action(IRECT(711,429,767,456),*this,[]{return std::string("Undo");},[this]{UndoClip(false);},[this]{return ClipEdits().nUndo>0 && view_.slots[selected_].ready;})));
  clipControls_.push_back(attach(new Action(IRECT(774,429,830,456),*this,[]{return std::string("Redo");},[this]{UndoClip(true);},[this]{return ClipEdits().nRedo>0 && view_.slots[selected_].ready;})));
  attach(new Action(IRECT(837,429,954,456),*this,[this]{return clipMode_?std::string("CLIP → TONE"):std::string("TONE → CLIP");},[this]{ToggleClipMode();}));
  toneControls_.push_back(attach(new Graph(IRECT(588,464,954,578),*this)));
  clipControls_.push_back(attach(new ClipGraph(IRECT(588,464,954,578),*this)));
  toneControls_.push_back(attach(new PointField(IRECT(588,585,704,613),*this,true)));
  toneControls_.push_back(attach(new PointField(IRECT(713,585,829,613),*this,false)));
  clipControls_.push_back(attach(new ClipField(IRECT(588,585,704,613),*this,false)));
  clipControls_.push_back(attach(new ClipField(IRECT(713,585,829,613),*this,true)));
  class CutoffReadout : public IControl {PadSampler& p_;public:CutoffReadout(IRECT r,PadSampler& p):IControl(r),p_(p){SetIgnoreMouse(true);}void Draw(IGraphics& g)override{auto c=p_.Shape();c.Materialize(p_.Exponent());int i=std::clamp(p_.pointSelection,0,c.count-1);g.DrawText(IText(12,precision::blue),(precision::Number(p_.ToneHz(c.points[i].x*127)/1000.,"%.2f")+" kHz").c_str(),mRECT);}};
  auto* cutoff=new CutoffReadout(IRECT(836,585,954,613),*this);g->AttachControl(cutoff);toneControls_.push_back(cutoff);
  auto* clipLength=new ClipLength(IRECT(836,585,954,613),*this);g->AttachControl(clipLength);clipControls_.push_back(clipLength);
  auto* toneHint=new ITextControl(IRECT(585,617,960,637),"Double-click adds · drag edits · Delete removes",IText(10,muted));g->AttachControl(toneHint);toneControls_.push_back(toneHint);
  auto* clipHint=new ITextControl(IRECT(585,617,960,637),"Space swaps handles · arrows 10 ms · Shift+arrows 1 frame",IText(10,muted));g->AttachControl(clipHint);clipControls_.push_back(clipHint);
  attach(new IVNumberBoxControl(IRECT(22,652,222,701),kAuditionVelocity,nullptr,"Audition velocity",style,true));
  button(IRECT(240,661,342,696),"Stop All",[this]{StopAll();});
  attach(new IVNumberBoxControl(IRECT(738,652,860,701),kMaster,nullptr,"Master · dB",style,true));
  attach(new Action(IRECT(874,662,954,695),*this,[this]{return reducedMotion_?"Motion Off":"Motion On";},[this]{reducedMotion_=!reducedMotion_;}));
  monitor_=new ITextControl(IRECT(356,652,724,701),"MIDI · awaiting input",IText(12,blue));g->AttachControl(monitor_);
  mapping_=new ITextControl(IRECT(22,706,956,724),"",IText(11,muted));g->AttachControl(mapping_);
  status_=new ITextControl(IRECT(22,731,956,751),"",IText(11,ink));g->AttachControl(status_);
  g->AttachControl(new FocusRing(g->GetBounds(),*this));
  help_=new precision::Help(g->GetBounds(),*this);g->AttachControl(help_);help_->Hide(true);
  g->SetKeyHandlerFunc([this](const IKeyPress& key,bool up){
    if(up || GetUI()->GetControlInTextEntry())return false;
    if(help_ && !help_->IsHidden()){if(key.VK==27 || key.VK==13){help_->Hide(true);GetUI()->SetAllControlsDirty();}return true;}
    if(key.VK==9){auto it=std::find(focusOrder_.begin(),focusOrder_.end(),focus_);int n=int(focusOrder_.size()),i=it==focusOrder_.end()?(key.S?0:-1):int(it-focusOrder_.begin());for(int tries=0;tries<n;++tries){i=(i+(key.S?-1:1)+n)%n;if(!focusOrder_[i]->IsHidden() && !focusOrder_[i]->IsDisabled()){focus_=focusOrder_[i];break;}}GetUI()->SetAllControlsDirty();return true;}
    if(!focus_)return false;
    if(focus_->OnKeyDown(0,0,key))return true;
    int param=focus_->GetParamIdx();
    if(param>=0 && (key.VK==38 || key.VK==40)){double value=std::clamp(GetParam(param)->GetNormalized()+(key.VK==38?1:-1)*(key.S?.001:.01),0.,1.);BeginInformHostOfParamChangeFromUI(param);SendParameterValueFromUI(param,value);EndInformHostOfParamChangeFromUI(param);focus_->SetValueFromDelegate(value);return true;}
    if(param>=0 && key.VK==13){focus_->PromptUserInput();return true;}return false;
  });
  Select(selected_);
}
