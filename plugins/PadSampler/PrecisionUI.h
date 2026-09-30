#pragma once
#include "IControls.h"
#include <cstdio>
#include <cstdlib>
namespace precision {
using namespace iplug; using namespace igraphics;
const IColor ink(255,25,32,42), muted(255,75,85,97), blue(255,20,110,224), silver(255,209,215,221), white(255,244,247,250), dark(255,31,38,47);
inline std::string Number(double n, const char* format="%.1f") { char s[64]; std::snprintf(s,sizeof(s),format,n); return s; }
inline void Metal(IGraphics& g, IRECT r, bool down=false) {
  const int v=down?185:235;
  g.PathClear();g.PathRoundRect(r,4);g.PathFill(IPattern::CreateLinearGradient(r,EDirection::Vertical,{{IColor(255,v,v+3,v+6),0.f},{IColor(255,v-35,v-32,v-29),1.f}}));
  g.DrawRoundRect(IColor(255,112,124,138),r,4,nullptr,1);
  g.DrawLine(white,r.L+3,r.T+1,r.R-3,r.T+1);
}
class Action : public IControl {
 public:
  Action(IRECT r,PadSampler& p,std::function<std::string()> label,std::function<void()> action,std::function<bool()> enabled=[] {return true;}) : IControl(r),p_(p),label_(label),action_(action),enabled_(enabled) {}
  void Draw(IGraphics& g) override {
    Metal(g,mRECT,down_); bool enabled=enabled_();
    if(mMouseIsOver && enabled) g.DrawRoundRect(blue,mRECT.GetPadded(-1),4,nullptr,1);
    if(p_.Focused()==this) g.DrawRoundRect(blue,mRECT.GetPadded(-3),3,nullptr,2);
    g.DrawText(IText(12,enabled?ink:muted),label_().c_str(),mRECT.GetTranslated(0,down_?1:0));
  }
  void OnMouseDown(float,float,const IMouseMod&) override { p_.Focus(this); down_=enabled_(); SetDirty(false); }
  void OnMouseUp(float x,float y,const IMouseMod&) override { bool fire=down_ && mRECT.Contains(x,y) && enabled_(); down_=false; SetDirty(false); if(fire) action_(); }
  void OnMouseOut() override { down_=false; IControl::OnMouseOut(); SetDirty(false); }
  bool OnKeyDown(float,float,const IKeyPress& k) override {if(p_.Focused()!=this)return false; if((k.VK==13 || k.VK==32) && enabled_()) { action_(); SetDirty(false); return true; } return false; }
 private: PadSampler& p_; std::function<std::string()> label_; std::function<void()> action_; std::function<bool()> enabled_; bool down_=false;
};
class Name : public IEditableTextControl {
 public:
  Name(IRECT r,PadSampler& p,std::function<void(const std::string&)> edit): IEditableTextControl(r,"",IText(19,ink)),p_(p),edit_(edit) {}
  void OnMouseDown(float x,float y,const IMouseMod& m) override { p_.Focus(this); IEditableTextControl::OnMouseDown(x,y,m); }
  void OnTextEntryCompletion(const char* s,int) override { edit_(s); SetStr(s); SetDirty(false); }
  bool OnKeyDown(float,float,const IKeyPress& k) override {if(p_.Focused()!=this)return false; if(k.VK==13) { GetUI()->CreateTextEntry(*this,mText,mRECT,GetStr()); return true; } return false; }
 private: PadSampler& p_; std::function<void(const std::string&)> edit_;
};
class Pad : public IControl {
 public:
  Pad(IRECT r,PadSampler& p,int slot):IControl(r),p_(p),slot_(slot) {}
  void Draw(IGraphics& g) override {
    auto& s=p_.View().slots[slot_]; bool selected=p_.Selected()==slot_;
    g.FillRoundRect(IColor(255,100,108,118),mRECT,12);
    g.FillRoundRect(IColor(255,18,24,31),mRECT.GetPadded(-3),10);
    const auto inner=mRECT.GetPadded(-7);
    g.PathClear();g.PathRoundRect(inner,7);g.PathFill(IPattern::CreateLinearGradient(inner,EDirection::Vertical,{{IColor(255,44,49,56),0.f},{IColor(255,32,37,44),1.f}}));
    g.DrawRoundRect(selected?blue:IColor(255,120,128,137),mRECT.GetPadded(-3),10,nullptr,selected?3:1);
    if(mMouseIsOver) g.DrawRoundRect(IColor(255,172,195,222),mRECT.GetPadded(-6),8,nullptr,1);
    g.DrawText(IText(16,white,nullptr,EAlign::Near),s.name.c_str(),IRECT(inner.L+9,inner.T+6,inner.R-35,inner.T+31));
    const float hit=p_.Hit(slot_); g.FillCircle(IColor(255,40+int(hit*100),90+int(hit*100),140+int(hit*100)),inner.R-16,inner.T+17,5);
    std::string filename=s.path.empty()?"Drop WAV / AIFF":std::filesystem::path(s.path).filename().string();
    g.DrawText(IText(12,white,nullptr,EAlign::Near),filename.c_str(),IRECT(inner.L+9,inner.T+34,inner.R-6,inner.T+54));
    auto wave=IRECT(inner.L+10,inner.T+59,inner.R-10,inner.B-29);
    for(int i=0;i<128;++i) { float x=wave.L+wave.W()*i/127.f,h=std::min(1.f,s.waveform[i])*wave.H()*.45f; g.DrawLine(selected?IColor(255,110,184,255):IColor(255,187,203,220),x,wave.MH()-h,x,wave.MH()+h,nullptr,1); }
    std::string status=s.status=="Drop WAV / AIFF or choose Load"?"Empty":s.status;
    g.DrawText(IText(10,IColor(255,193,205,219),nullptr,EAlign::Near),status.c_str(),IRECT(inner.L+9,inner.B-26,inner.R-62,inner.B-5));
    if(selected) g.DrawText(IText(10,IColor(255,106,184,255)),"Selected",IRECT(inner.R-66,inner.B-26,inner.R-4,inner.B-5));
  }
  void OnMouseDown(float,float,const IMouseMod&) override { p_.Focus(this); p_.Select(slot_); p_.Audition(slot_); }
  bool OnKeyDown(float,float,const IKeyPress& k) override {if(p_.Focused()!=this)return false; if(k.VK==13 || k.VK==32) {p_.Select(slot_);p_.Audition(slot_);return true;} return false; }
  void OnDrop(const char* path) override {p_.Select(slot_);p_.Load(slot_,path);}
  void OnDropMultiple(const std::vector<const char*>& paths) override { if(paths.size()==1) OnDrop(paths[0]); else p_.Message("Drop one WAV or AIFF per pad"); }
 private: PadSampler& p_;int slot_;
};
class Graph : public IControl {
 public:
  Graph(IRECT r,PadSampler& p):IControl(r),p_(p) {}
  IRECT Plot() const {return IRECT(mRECT.L+27,mRECT.T+18,mRECT.R-8,mRECT.B-22);}
  void Draw(IGraphics& g) override {
    auto c=p_.Shape(); auto r=Plot(); bool bypass=p_.GetParam(p_.Selected()*9+ToneBypass)->Bool();
    g.FillRoundRect(dark,mRECT,5);
    for(int i=0;i<5;++i) {float x=r.L+r.W()*i/4.f,y=r.T+r.H()*i/4.f;g.DrawLine(IColor(255,61,74,90),x,r.T,x,r.B);g.DrawLine(IColor(255,61,74,90),r.L,y,r.R,y);}
    auto color=bypass?IColor(255,139,153,169):IColor(255,89,174,255);
    for(int i=1;i<=127;++i) g.DrawLine(color,r.L+r.W()*(i-1)/127.f,r.B-r.H()*c.Evaluate((i-1)/127.,p_.Exponent()),r.L+r.W()*i/127.f,r.B-r.H()*c.Evaluate(i/127.,p_.Exponent()),nullptr,2);
    auto points=c; points.Materialize(p_.Exponent());
    for(int i=0;i<points.count;++i) {auto pt=points.points[i]; float x=r.L+pt.x*r.W(),y=r.B-pt.y*r.H();g.FillCircle(i==p_.pointSelection?white:blue,x,y,4);if(i==p_.pointSelection)g.DrawCircle(blue,x,y,7,nullptr,1);}
    int velocity=p_.LastHit(p_.Selected()); if(velocity) {float x=r.L+r.W()*velocity/127.f;g.DrawLine(IColor(255,179,214,250),x,r.T,x,r.B,nullptr,1);}
    g.DrawText(IText(10,white,nullptr,EAlign::Near),bypass?"BRIGHTNESS · BYPASSED":c.legacy?"BRIGHTNESS · LEGACY":"BRIGHTNESS %",mRECT.GetPadded(-6).GetFromTop(14));
    g.DrawText(IText(10,white),"0        Hit velocity        127",mRECT.GetFromBottom(19));
    g.DrawText(IText(9,white),"100",IRECT(mRECT.L,r.T-4,r.L-2,r.T+9));g.DrawText(IText(9,white),"0",IRECT(mRECT.L,r.B-10,r.L-2,r.B+3));
  }
  int Nearest(float x,float y) const {auto c=p_.Shape();c.Materialize(p_.Exponent());auto r=Plot(); for(int i=0;i<c.count;++i) if(std::hypot(x-(r.L+c.points[i].x*r.W()),y-(r.B-c.points[i].y*r.H()))<11) return i;return -1;}
  void OnMouseDown(float x,float y,const IMouseMod&) override {p_.Focus(this); before_=p_.Shape(); epoch_=p_.CurveEpoch(); int hit=Nearest(x,y); if(hit>=0) p_.pointSelection=hit; dragging_=hit>0; SetDirty(false);}
  void OnMouseDrag(float x,float y,float,float,const IMouseMod&) override {if(epoch_!=p_.CurveEpoch())dragging_=false;if(!dragging_)return;auto c=p_.Shape();c.Materialize(p_.Exponent());auto r=Plot();if(c.Move(p_.pointSelection,(x-r.L)/r.W(),(r.B-y)/r.H()))p_.SetShape(c);}
  void OnMouseUp(float,float,const IMouseMod&) override {if(dragging_ && epoch_==p_.CurveEpoch())p_.CommitShape(before_);dragging_=false;}
  void OnMouseDblClick(float x,float y,const IMouseMod&) override { if(Nearest(x,y)>=0)return; auto before=p_.Shape(),c=before;c.Materialize(p_.Exponent());auto r=Plot();int index=c.Add((x-r.L)/r.W(),(r.B-y)/r.H());if(index>=0){p_.pointSelection=index;p_.SetShape(c);p_.CommitShape(before);}else p_.Message("Maximum 8 points; endpoints remain fixed");}
  bool OnKeyDown(float,float,const IKeyPress& k) override {if(p_.Focused()!=this)return false;
    auto before=p_.Shape(),c=before;c.Materialize(p_.Exponent());int i=std::clamp(p_.pointSelection,0,c.count-1);bool change=false;
    if(k.VK==8 || k.VK==46) change=c.Remove(i);
    else if(k.VK>=37 && k.VK<=40) {double d=k.S? .001:.01;change=c.Move(i,c.points[i].x+(k.VK==39?d:k.VK==37?-d:0),c.points[i].y+(k.VK==38?d:k.VK==40?-d:0));}
    else if(k.VK==27 && dragging_) {p_.SetShape(before_);dragging_=false;return true;} else return false;
    if(change){p_.SetShape(c);p_.CommitShape(before);p_.pointSelection=std::min(i,c.count-2);}return true;
  }
 private:PadSampler& p_;padsampler::ToneShape before_;bool dragging_=false;unsigned epoch_=0;
};
class PointField : public IControl {
 public:
  PointField(IRECT r,PadSampler& p,bool velocity):IControl(r),p_(p),velocity_(velocity) {}
  void Draw(IGraphics& g) override {auto c=p_.Shape();c.Materialize(p_.Exponent());int i=std::clamp(p_.pointSelection,0,c.count-1);Metal(g,mRECT);auto v=velocity_? c.points[i].x*127:c.points[i].y*100;g.DrawText(IText(11,ink),(std::string(velocity_?"Velocity ":"Bright % ")+Number(v)).c_str(),mRECT);}
  void Edit() {auto c=p_.Shape();c.Materialize(p_.Exponent());int i=std::clamp(p_.pointSelection,0,c.count-1);if(i==0 || i==c.count-1){p_.Message("Endpoints are fixed; select an interior point");return;}GetUI()->CreateTextEntry(*this,IText(13,ink),mRECT,Number(velocity_?c.points[i].x*127:c.points[i].y*100).c_str());}
  void OnMouseDown(float,float,const IMouseMod&) override {p_.Focus(this);Edit();}
  bool OnKeyDown(float,float,const IKeyPress& k) override {if(p_.Focused()!=this)return false;if(k.VK==13){Edit();return true;}return false;}
  void OnTextEntryCompletion(const char* str,int) override {char* end=nullptr;double value=std::strtod(str,&end);if(end==str || *end || !std::isfinite(value)){p_.Message("Enter a finite numeric point value");return;}auto before=p_.Shape(),c=before;c.Materialize(p_.Exponent());int i=std::clamp(p_.pointSelection,0,c.count-1);if(c.Move(i,velocity_?value/127:c.points[i].x,velocity_?c.points[i].y:value/100)){p_.SetShape(c);p_.CommitShape(before);}}
 private:PadSampler& p_;bool velocity_;
};
class Presets : public Action {
 public:
  Presets(IRECT r,PadSampler& p):Action(r,p,[&p]{auto c=p.Shape();if(c.legacy)return std::string("Legacy exponent");const char* names[]={"Linear","Early Open","Late Open","S-Curve"};for(int i=0;i<4;++i)if(c==padsampler::ToneShape::Preset(i))return std::string(names[i]);if(c==padsampler::ToneShape{})return std::string("Linear");return std::string("Custom");},[this]{menu_.SetChosenItemIdx(-1);GetUI()->CreatePopupMenu(*this,menu_,mRECT);}),p_(p) {for(auto name:{"Linear","Early Open","Late Open","S-Curve"})menu_.AddItem(name);}
  void OnPopupMenuSelection(IPopupMenu* menu,int) override {if(menu && menu->GetChosenItemIdx()>=0)p_.CurvePreset(menu->GetChosenItemIdx());}
 private:PadSampler& p_;IPopupMenu menu_; // macOS popup completion is asynchronous.
};
class Knob : public IVKnobControl {
 public:
  Knob(IRECT r,int param,const char* label,const IVStyle& style,PadSampler& p):IVKnobControl(r,param,label,style,true),p_(p) {SetInnerPointerFrac(.3);SetOuterPointerFrac(.9);SetPointerThickness(2);}
  void OnMouseDown(float x,float y,const IMouseMod& m) override {p_.Focus(this);IVKnobControl::OnMouseDown(x,y,m);}
  void OnResize() override {
    IVKnobControl::OnResize();
    mLabelBounds=mRECT.GetFromTop(18);mValueBounds=mRECT.GetFromBottom(18);
    mWidgetBounds=IRECT(mRECT.L,mRECT.T+20,mRECT.R,mRECT.B-20).GetCentredInside(58,58);
  }
  void Draw(IGraphics& g) override {DrawLabel(g);DrawWidget(g);DrawValue(g,mValueMouseOver);}
  void DrawHandle(IGraphics& g,const IRECT& b) override {
    const float cx=b.MW(),cy=b.MH(),r=b.W()/2;
    g.FillCircle(IColor(70,0,0,0),cx+1,cy+2,r+2);
    g.FillCircle(dark,cx,cy,r+1);
    for(int i=int(r);i>1;--i) {int shade=155+int(77*(1.-i/r));g.FillCircle(IColor(255,shade,shade+3,shade+6),cx,cy,i-1);}
    g.DrawArc(white,cx,cy,r-2,-100,70,nullptr,1.5);
    g.DrawArc(muted,cx,cy,r-2,80,260,nullptr,1);
  }
  void DrawIndicatorTrack(IGraphics& g,float angle,float cx,float cy,float radius) override {g.DrawArc(IColor(255,151,164,180),cx,cy,radius,-135,135,nullptr,3);g.DrawArc(blue,cx,cy,radius,-135,angle,nullptr,3);}
 private:PadSampler& p_;
};
class Backplate : public IControl {
 public:Backplate(IRECT r,IBitmap bitmap):IControl(r),bitmap_(bitmap){SetIgnoreMouse(true);}
  void Draw(IGraphics& g) override {if(bitmap_.IsValid())g.DrawBitmap(bitmap_,mRECT);else Metal(g,mRECT);g.DrawRoundRect(muted,mRECT.GetPadded(-1),7,nullptr,1);g.DrawLine(muted,568,78,568,640);g.DrawLine(muted,16,69,964,69);g.DrawLine(muted,16,642,964,642);}
 private:IBitmap bitmap_;
};
class FocusRing : public IControl {
 public:FocusRing(IRECT r,PadSampler& p):IControl(r),p_(p){SetIgnoreMouse(true);}
  void Draw(IGraphics& g) override {if(auto* c=p_.Focused())if(!c->IsHidden())g.DrawRoundRect(blue,c->GetRECT().GetPadded(2),4,nullptr,1.5);}
 private:PadSampler& p_;
};
class Help : public IControl {
 public:Help(IRECT r,PadSampler& p):IControl(r),p_(p) {}
  void Draw(IGraphics& g) override {
    const IRECT r(90,98,890,660);
    g.FillRect(IColor(125,22,30,42),mRECT);
    g.FillRoundRect(white,r,9);g.DrawRoundRect(blue,r,9,nullptr,2);
    g.DrawText(IText(24,ink),"PADSAMPLER / PRECISION",r.GetFromTop(60));
    const char* lines[]={"Drop WAV / AIFF onto a pad. Click or press Space to audition.","Use MIDI Learn, then strike the hardware pad. Click again to cancel.","Standalone Settings selects audio / MIDI. In a plugin, use host routing.","Soft and Hard brightness set the low-pass cutoff range.","Double-click the graph to add a point. Drag to shape the response.","Select a point: arrows move, Shift makes fine edits, Delete removes.","Endpoints are fixed. Curves rise monotonically; flat sections are allowed.","Presets replace this pad's curve. Undo / Redo remembers 32 edits.","Curve edits affect NEXT hits; already ringing samples stay unchanged.","Curves save in kits / sessions, but points are not DAW automation.","Legacy exponent automation applies only while the curve says Legacy.","The first point edit converts legacy mode. Undo restores it.","Bypass compares tone; Velocity Volume controls loudness separately.","Tab moves focus. Enter edits numeric fields. Motion toggles hit flashes.","Open Kit selects kit.json. Save Kit collects samples; relink missing files."};
    for(int i=0;i<15;++i)g.DrawText(IText(14,ink,nullptr,EAlign::Near),lines[i],IRECT(r.L+24,r.T+65+i*26,r.R-24,r.T+89+i*26));
    g.DrawText(IText(17,blue),"Close · Escape",r.GetFromBottom(52));
  }
  void OnMouseDown(float x,float y,const IMouseMod&) override {if(IRECT(90,600,890,660).Contains(x,y)){Hide(true);GetUI()->SetAllControlsDirty();}}
 private:PadSampler& p_;
};
}
