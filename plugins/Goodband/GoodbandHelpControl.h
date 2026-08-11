#pragma once

#include <algorithm>

#include "IControls.h"

class GoodbandHelpControl final : public IControl {
public:
  explicit GoodbandHelpControl(const IRECT& bounds) : IControl(bounds) {}

  void Draw(IGraphics& graphics) override {
    if (isOpen_) {
      DrawPanel(graphics);
    } else {
      DrawHelpButton(graphics);
    }
  }

  bool IsHit(float x, float y) const override {
    return isOpen_ || HelpButtonBounds().Contains(x, y);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override {
    if (!isOpen_) {
      isOpen_ = true;
    } else if (CloseButtonBounds().Contains(x, y) || !PanelBounds().Contains(x, y)) {
      isOpen_ = false;
    }
    SetDirty(false);
  }

private:
  const IColor kInk{248, 9, 10, 10};
  const IColor kDeepInk{255, 17, 16, 14};
  const IColor kGold{255, 222, 174, 86};
  const IColor kSoftGold{255, 191, 160, 108};
  const IColor kIvory{255, 239, 226, 197};
  const IColor kMuted{255, 190, 183, 165};
  const IColor kJade{255, 103, 226, 184};

  IRECT PanelBounds() const { return IRECT(X(32.0F), Y(30.0F), X(688.0F), Y(410.0F)); }
  IRECT HelpButtonBounds() const { return IRECT(X(674.0F), Y(17.0F), X(706.0F), Y(49.0F)); }
  IRECT CloseButtonBounds() const { return IRECT(X(646.0F), Y(43.0F), X(676.0F), Y(73.0F)); }

  void DrawHelpButton(IGraphics& graphics) const {
    const auto button = HelpButtonBounds();
    graphics.FillCircle(kDeepInk, button.MW(), button.MH(), button.W() * 0.47F);
    graphics.DrawCircle(kSoftGold, button.MW(), button.MH(), button.W() * 0.47F, nullptr, Scale(1.3F));
    graphics.DrawCircle(IColor(180, 89, 205, 167), button.MW(), button.MH(), button.W() * 0.38F, nullptr,
                        Scale(0.8F));
    DrawText(graphics, "?", 17.0F, kIvory, button, EAlign::Center);
  }

  void DrawPanel(IGraphics& graphics) const {
    const auto panel = PanelBounds();
    graphics.FillRect(IColor(190, 0, 0, 0), mRECT);
    graphics.FillRect(kInk, panel);
    graphics.DrawRect(kGold, panel, nullptr, Scale(1.6F));
    graphics.DrawRect(IColor(220, 91, 72, 38), panel.GetPadded(-5.0F), nullptr, Scale(0.8F));

    DrawText(graphics, "THE THREEFOLD MANUAL", 22.0F, kIvory,
             IRECT(X(55.0F), Y(42.0F), X(500.0F), Y(76.0F)));
    DrawText(graphics, "MASTERING ENERGY / QUICK GUIDE", 10.5F, kSoftGold,
             IRECT(X(56.0F), Y(74.0F), X(420.0F), Y(94.0F)));
    graphics.FillRect(kGold, IRECT(X(56.0F), Y(98.0F), X(664.0F), Y(99.4F)));

    graphics.FillCircle(kDeepInk, CloseButtonBounds().MW(), CloseButtonBounds().MH(), Scale(13.0F));
    graphics.DrawCircle(kSoftGold, CloseButtonBounds().MW(), CloseButtonBounds().MH(), Scale(13.0F), nullptr,
                        Scale(1.0F));
    DrawText(graphics, "X", 11.0F, kIvory, CloseButtonBounds(), EAlign::Center);

    DrawText(graphics, "WHAT IT DOES", 11.5F, kJade,
             IRECT(X(56.0F), Y(108.0F), X(250.0F), Y(130.0F)));
    DrawText(graphics, "Three-band compression, harmonic saturation and tonal shaping for controlled energy.",
             12.4F, kIvory, IRECT(X(56.0F), Y(130.0F), X(664.0F), Y(151.0F)));
    DrawText(graphics, "Use it on a mix, drum bus or instrument bus when the sound needs cohesion, color, punch or width.",
             11.2F, kMuted, IRECT(X(56.0F), Y(151.0F), X(664.0F), Y(172.0F)));

    DrawText(graphics, "CHARACTER", 11.5F, kJade,
             IRECT(X(56.0F), Y(184.0F), X(320.0F), Y(206.0F)));
    DrawMode(graphics, 205.0F, "CLEAN", "Balanced, transparent control");
    DrawMode(graphics, 234.0F, "WARM", "Low-mid weight and harmonic color");
    DrawMode(graphics, 263.0F, "PUNCH", "Transient-forward impact and recovery");
    DrawMode(graphics, 292.0F, "WIDE", "High-band sheen plus stereo side lift");

    DrawText(graphics, "CONTROLS", 11.5F, kJade,
             IRECT(X(374.0F), Y(184.0F), X(650.0F), Y(206.0F)));
    DrawControl(graphics, 205.0F, "AMOUNT", "Compression, saturation and character strength");
    DrawControl(graphics, 244.0F, "MIX", "Parallel blend between dry and processed signal");
    DrawControl(graphics, 283.0F, "OUTPUT", "Final gain trim for honest level matching");

    graphics.FillRect(IColor(255, 84, 65, 35), IRECT(X(56.0F), Y(329.0F), X(664.0F), Y(330.0F)));
    DrawText(graphics, "FORMS / METERS / MATCH", 10.8F, kGold,
             IRECT(X(56.0F), Y(339.0F), X(250.0F), Y(360.0F)));
    DrawText(graphics, "Forms load complete starting stances. LOW, MID and HIGH show live gain reduction.",
             11.0F, kIvory, IRECT(X(56.0F), Y(360.0F), X(664.0F), Y(379.0F)));
    DrawText(graphics, "MATCH compensates perceived level; the GAIN COMP plaque shows the trim applied before OUTPUT.",
             10.7F, kMuted, IRECT(X(56.0F), Y(379.0F), X(664.0F), Y(398.0F)));
  }

  void DrawMode(IGraphics& graphics, float y, const char* name, const char* description) const {
    DrawText(graphics, name, 11.5F, kGold, IRECT(X(56.0F), Y(y), X(113.0F), Y(y + 20.0F)));
    DrawText(graphics, description, 10.6F, kMuted,
             IRECT(X(116.0F), Y(y), X(350.0F), Y(y + 20.0F)));
  }

  void DrawControl(IGraphics& graphics, float y, const char* name, const char* description) const {
    DrawText(graphics, name, 11.5F, kGold, IRECT(X(374.0F), Y(y), X(664.0F), Y(y + 19.0F)));
    DrawText(graphics, description, 10.5F, kMuted,
             IRECT(X(374.0F), Y(y + 17.0F), X(664.0F), Y(y + 35.0F)));
  }

  void DrawText(IGraphics& graphics, const char* text, float size, const IColor& color, const IRECT& bounds,
                EAlign align = EAlign::Near) const {
    graphics.DrawText(IText(Scale(size), color, DEFAULT_FONT, align, EVAlign::Middle), text, bounds);
  }

  float X(float coordinate) const { return mRECT.L + coordinate * (mRECT.W() / 720.0F); }
  float Y(float coordinate) const { return mRECT.T + coordinate * (mRECT.H() / 440.0F); }
  float Scale(float amount) const { return amount * std::min(mRECT.W() / 720.0F, mRECT.H() / 440.0F); }

  bool isOpen_ = false;
};
