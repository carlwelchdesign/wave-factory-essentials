#include "PadSampler.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "../shared/WaveFactoryUI.h"
#include <filesystem>
using namespace iplug;
using namespace igraphics;
using namespace padsampler;
namespace {
const IColor kInk(255, 229, 234, 232), kMint(255, 129, 229, 191), kMuted(255, 155, 170, 164);
class PadControl final : public IControl {
 public:
  PadControl(IRECT rect, PadSampler& owner, int slot) : IControl(rect), owner_(owner), slot_(slot) {}
  void Draw(IGraphics& g) override {
    const auto& s = owner_.View().slots[slot_];
    g.FillRoundRect(IColor(255, 32 + int(owner_.Hit(slot_) * 30), 43 + int(owner_.Hit(slot_) * 60), 43 + int(owner_.Hit(slot_) * 45)), mRECT, 12);
    g.DrawRoundRect(owner_.Selected() == slot_ ? kMint : IColor(255, 61, 78, 75), mRECT.GetPadded(-1), 12, nullptr, 2);
    g.DrawText(IText(17, kInk), s.name.c_str(), mRECT.GetFromTop(37));
    std::string file = s.path.empty() ? "Drop WAV / AIFF here" : std::filesystem::path(s.path).filename().string();
    g.DrawText(IText(12, kMuted), file.c_str(), mRECT.GetFromTop(63).GetFromBottom(24));
    const auto wave = mRECT.GetPadded(-14).GetFromBottom(mRECT.H() - 84);
    for (int b = 0; b < 128; ++b) {
      float x = wave.L + wave.W() * b / 127.f;
      float h = std::min(1.f, s.waveform[b]) * wave.H() * .45f;
      g.DrawLine(kMint, x, wave.MH() - h, x, wave.MH() + h, nullptr, 1);
    }
  }
  void OnMouseDown(float, float, const IMouseMod&) override { owner_.Select(slot_); owner_.Audition(slot_); }
  void OnDrop(const char* path) override { owner_.Select(slot_); owner_.Load(slot_, path); }
  void OnDropMultiple(const std::vector<const char*>& paths) override { if (paths.size() == 1) OnDrop(paths[0]); }
 private: PadSampler& owner_; int slot_;
};
class NameControl final : public IEditableTextControl {
 public:
  NameControl(IRECT r, std::function<void(const std::string&)> rename) : IEditableTextControl(r, "", IText(22, kInk)), rename_(std::move(rename)) {}
  void OnTextEntryCompletion(const char* str, int) override { rename_(str); SetStr(str); SetDirty(false); }
 private: std::function<void(const std::string&)> rename_;
};
class Help final : public IControl {
 public:
  explicit Help(IRECT r) : IControl(r) {}
  void Draw(IGraphics& g) override {
    g.FillRoundRect(IColor(255, 17, 26, 26), mRECT, 14);
    g.DrawRoundRect(kMint, mRECT, 14, nullptr, 2);
    g.DrawText(IText(26, kInk), "PLAY THE TONE", mRECT.GetFromTop(70));
    const char* lines[] = {"1  Connect SamplePad 4 by USB. Use the computer's audio output.", "2  Standalone: choose MIDI input and audio device in Preferences.", "    In Logic: load PadSampler as a software instrument and route MIDI.", "3  Drop one WAV/AIFF onto each pad. Click to select and audition.", "4  Select a pad, press MIDI Learn, then strike its matching trigger.", "    Learn Tip and Ring separately. Duplicate notes are flagged.", "5  Soft / Hard brightness set how velocity opens the low-pass filter.", "    Velocity Volume controls loudness separately. Bypass compares tone.", "6  Save Kit collects samples. Open Kit selects kit.json inside .padkit.", "    Use Load to relink a missing sample. Stop All fades ringing voices.", "Start with a 128-frame audio buffer. Watch output levels when layering.", "This is a single-sample instrument, not recorded velocity layers."};
    for (int i = 0; i < 12; ++i) g.DrawText(IText(16, kInk), lines[i], IRECT(mRECT.L + 18, mRECT.T + 80 + i * 28, mRECT.R - 18, mRECT.T + 106 + i * 28));
    g.DrawText(IText(20, kMint), "Close  ·  Escape", mRECT.GetFromBottom(65));
  }
  void OnMouseDown(float x, float y, const IMouseMod&) override { if (mRECT.GetFromBottom(65).Contains(x,y)) { Hide(true); GetUI()->SetAllControlsDirty(); } }
  bool OnKeyDown(float, float, const IKeyPress& key) override { if (key.VK == 27) { Hide(true); GetUI()->SetAllControlsDirty(); return true; } return false; }
};
}
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
  mLayoutFunc = [&](IGraphics* g) {
    g->LoadFont(DEFAULT_FONT, "Arial", ETextStyle::Normal);
    g->AttachPanelBackground(IColor(255, 17, 23, 24));
    g->AttachTextEntryControl();
    g->SetKeyHandlerFunc([this](const IKeyPress& key, bool up) {
      if (!up && key.VK == 27 && help_ && !help_->IsHidden()) { help_->Hide(true); GetUI()->SetAllControlsDirty(); return true; }
      return false;
    });
    g->AttachControl(new ITextControl(IRECT(25, 15, 430, 60), "PADSAMPLER  /  WAVE FACTORY", IText(23, kInk)));
    auto style = wfe::ui::MakeDarkControlStyle(kMint);
    auto button = [&](IRECT r, const char* text, IActionFunction action) { g->AttachControl(new IVButtonControl(r, [action](IControl* c) { SplashClickActionFunc(c); action(c); }, text, style)); };
    button(IRECT(600, 18, 705, 55), "Open Kit", [this](IControl*) { ChooseKit(false); });
    button(IRECT(715, 18, 820, 55), "Save Kit", [this](IControl*) { ChooseKit(true); });
    button(IRECT(830, 18, 955, 55), "? Help", [this](IControl*) { help_->Hide(!help_->IsHidden()); GetUI()->SetAllControlsDirty(); });
    // Four physical pads: narrow upper pair, larger lower pair. External slots remain distinct.
    const IRECT pads[] = {IRECT(25, 95, 285, 205), IRECT(300, 95, 560, 205), IRECT(25, 220, 285, 405), IRECT(300, 220, 560, 405), IRECT(25, 425, 285, 550), IRECT(300, 425, 560, 550)};
    for (int i = 0; i < kSlots; ++i) g->AttachControl(new PadControl(pads[i], *this, i));
    g->AttachControl(name_ = new NameControl(IRECT(595, 90, 955, 130), [this](const std::string& name) { library_.Rename(selected_, name); }));
    button(IRECT(595, 138, 725, 177), "Load / Relink", [this](IControl*) { ChooseSample(); });
    button(IRECT(735, 138, 860, 177), "MIDI Learn", [this](IControl*) { engine_.learn.store(selected_); });
    button(IRECT(870, 138, 955, 177), "Clear", [this](IControl*) { library_.Clear(selected_); });
    g->AttachControl(controls_[Note] = new IVNumberBoxControl(IRECT(595, 185, 770, 245), Note, nullptr, "MIDI note", style, true));
    g->AttachControl(controls_[Channel] = new IVNumberBoxControl(IRECT(780, 185, 955, 245), Channel, nullptr, "Channel · 0 = All", style, true));
    const char* labels[] = {"Level", "Pan", "Velocity Volume", "Soft Brightness", "Hard Brightness", "Tone Curve"};
    for (int i = 0; i < 6; ++i) {
      float x = 592.f + (i % 3) * 122.f, y = 270.f + (i / 3) * 145.f;
      g->AttachControl(controls_[i] = new IVKnobControl(IRECT(x, y, x + 118, y + 138), i, labels[i], style, true));
    }
    g->AttachControl(controls_[ToneBypass] = new IVSwitchControl(IRECT(600, 570, 775, 625), ToneBypass, "Tone Bypass", style));
    button(IRECT(795, 578, 955, 620), "Stop All", [this](IControl*) { engine_.stop.store(true); });
    g->AttachControl(new IVNumberBoxControl(IRECT(25, 575, 255, 635), kAuditionVelocity, nullptr, "Audition velocity", style, true));
    g->AttachControl(new IVNumberBoxControl(IRECT(300, 575, 560, 635), kMaster, nullptr, "Master level · dB", style, true));
    g->AttachControl(mapping_ = new ITextControl(IRECT(20, 645, 960, 670), "", IText(13, kMint)));
    g->AttachControl(monitor_ = new ITextControl(IRECT(20, 675, 960, 700), "", IText(14, kMuted)));
    g->AttachControl(status_ = new ITextControl(IRECT(20, 705, 960, 744), "", IText(13, kInk)));
    g->AttachControl(help_ = new Help(IRECT(90, 105, 890, 650))); help_->Hide(true);
    Select(selected_);
  };
}
PadSampler::~PadSampler() { library_.Stop(); engine_.Shutdown(); }
void PadSampler::OnReset() { engine_.Reset(GetSampleRate()); }
void PadSampler::ProcessMidiMsg(const IMidiMsg& msg) {
  if (msg.StatusMsg() == IMidiMsg::kNoteOn && msg.Velocity() > 0) engine_.Midi(msg.mOffset, msg.NoteNumber(), msg.Channel() + 1, msg.Velocity());
  if (msg.StatusMsg() == IMidiMsg::kControlChange && (msg.ControlChangeIdx() == 120 || msg.ControlChangeIdx() == 123)) engine_.stop.store(true);
}
void PadSampler::ProcessBlock(sample**, sample** outputs, int frames) {
  for (int i = 0; i < kSlots; ++i) {
    int p = i * kSlotParams; auto& s = engine_.settings[i];
    s.level = GetParam(p + Level)->Value(); s.pan = GetParam(p + Pan)->Value();
    s.velocityAmount = GetParam(p + VelocityAmount)->Value(); s.softHz = GetParam(p + SoftHz)->Value();
    s.hardHz = GetParam(p + HardHz)->Value(); s.curve = GetParam(p + ToneCurve)->Value();
    s.bypass = GetParam(p + ToneBypass)->Bool(); s.note = GetParam(p + Note)->Int(); s.channel = GetParam(p + Channel)->Int();
  }
  engine_.master = std::pow(10., GetParam(kMaster)->Value() / 20.);
  if (NOutChansConnected() >= 2) engine_.Process(outputs[0], outputs[1], frames);
}
void PadSampler::Select(int slot) {
  selected_ = slot;
  if (!GetUI()) return;
  for (int i = 0; i < kSlotParams; ++i) if (controls_[i]) { controls_[i]->SetParamIdx(slot * kSlotParams + i); controls_[i]->SetValueFromDelegate(GetParam(slot * kSlotParams + i)->GetNormalized()); }
  name_->SetStr(view_.slots[slot].name.c_str()); GetUI()->SetAllControlsDirty();
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
    if (save) library_.SaveKit(path.Get(), Parameters()); else library_.OpenKit(path.Get());
  });
}
std::vector<double> PadSampler::Parameters() const { std::vector<double> values; for (int i = 0; i < kParameterCount; ++i) values.push_back(GetParam(i)->Value()); return values; }
void PadSampler::ApplyParameters(const Json& doc, bool notifyHost) {
  for (int i = 0; i < kParameterCount; ++i) {
    GetParam(i)->Set(doc["parameters"][i].get<double>());
    if (notifyHost) { BeginInformHostOfParamChangeFromUI(i); SendParameterValueFromUI(i, GetParam(i)->GetNormalized()); EndInformHostOfParamChangeFromUI(i); }
  }
}
bool PadSampler::SerializeState(IByteChunk& chunk) const {
  auto doc = library_.Document(Parameters()).dump(); int length = int(doc.size());
  chunk.Put(&length); chunk.PutBytes(doc.data(), length); return true;
}
int PadSampler::UnserializeState(const IByteChunk& chunk, int position) {
  int length = 0; position = chunk.Get(&length, position);
  if (position < 0 || length < 0 || length > 65536 || length > chunk.Size() - position) return -1;
  try {
    std::string text(size_t(length), '\0'); int end = chunk.GetBytes(text.data(), length, position);
    auto doc = Json::parse(text); ValidateDocument(doc); ApplyParameters(doc, false); library_.Restore(doc); return end;
  } catch (...) { return -1; }
}
void PadSampler::OnIdle() {
  Json doc; if (library_.TakeReady(doc)) { ApplyParameters(doc, true); if (GetUI()) Select(selected_); }
  int learned = engine_.learned.exchange(-1);
  if (learned >= 0) {
    int p = ((learned >> 24) & 7) * kSlotParams;
    for (auto item : {std::pair<int,int>{p + Note, learned & 127}, {p + Channel, (learned >> 8) & 31}}) {
      GetParam(item.first)->Set(item.second); BeginInformHostOfParamChangeFromUI(item.first); SendParameterValueFromUI(item.first, GetParam(item.first)->GetNormalized()); EndInformHostOfParamChangeFromUI(item.first);
    }
    if (GetUI()) Select(selected_);
  }
  view_ = library_.View();
  for (int i = 0; i < kSlots; ++i) flashes_[i] = std::max(flashes_[i] * .8f, engine_.hits[i].exchange(0) / 127.f);
  if (!GetUI()) return;
  name_->SetStr(view_.slots[selected_].name.c_str());
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
  status_->SetStr(status.c_str()); GetUI()->SetAllControlsDirty();
}
