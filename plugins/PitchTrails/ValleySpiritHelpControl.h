#pragma once

#include <algorithm>

#include "IControls.h"

class ValleySpiritHelpControl final : public IControl {
public:
  explicit ValleySpiritHelpControl(const IRECT& bounds) : IControl(bounds) {}

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
  const IColor kInk{248, 7, 10, 17};
  const IColor kDeepInk{255, 15, 18, 25};
  const IColor kBronze{255, 185, 145, 91};
  const IColor kSoftBronze{255, 151, 124, 87};
  const IColor kMoonstone{255, 228, 235, 242};
  const IColor kMuted{255, 170, 178, 190};
  const IColor kJade{255, 137, 184, 167};

  IRECT PanelBounds() const { return IRECT(X(34.0F), Y(30.0F), X(726.0F), Y(430.0F)); }
  IRECT HelpButtonBounds() const { return IRECT(X(190.0F), Y(411.0F), X(222.0F), Y(443.0F)); }
  IRECT CloseButtonBounds() const { return IRECT(X(680.0F), Y(43.0F), X(710.0F), Y(73.0F)); }

  void DrawHelpButton(IGraphics& graphics) const {
    const auto button = HelpButtonBounds();
    graphics.FillCircle(kDeepInk, button.MW(), button.MH(), button.W() * 0.47F);
    graphics.DrawCircle(kBronze, button.MW(), button.MH(), button.W() * 0.47F, nullptr,
                        Scale(1.2F));
    graphics.DrawCircle(IColor(190, 119, 169, 155), button.MW(), button.MH(),
                        button.W() * 0.37F, nullptr, Scale(0.8F));
    DrawText(graphics, "?", 17.0F, kMoonstone, button, EAlign::Center, VALLEY_SPIRIT_FONT);
  }

  void DrawPanel(IGraphics& graphics) const {
    const auto panel = PanelBounds();
    graphics.FillRect(IColor(194, 0, 0, 0), mRECT);
    graphics.FillRect(kInk, panel);
    graphics.DrawRect(kBronze, panel, nullptr, Scale(1.5F));
    graphics.DrawRect(IColor(220, 68, 73, 78), panel.GetPadded(-5.0F), nullptr, Scale(0.8F));

    DrawText(graphics, "THE VALLEY SPIRIT MANUAL", 22.0F, kMoonstone,
             IRECT(X(58.0F), Y(43.0F), X(550.0F), Y(77.0F)), EAlign::Near,
             VALLEY_SPIRIT_FONT);
    DrawText(graphics, "PITCH-SHIFTING DELAY / RETURNING ENERGY", 10.5F, kBronze,
             IRECT(X(59.0F), Y(75.0F), X(500.0F), Y(95.0F)));
    graphics.FillRect(kBronze, IRECT(X(59.0F), Y(100.0F), X(695.0F), Y(101.2F)));

    graphics.FillCircle(kDeepInk, CloseButtonBounds().MW(), CloseButtonBounds().MH(), Scale(13.0F));
    graphics.DrawCircle(kSoftBronze, CloseButtonBounds().MW(), CloseButtonBounds().MH(),
                        Scale(13.0F), nullptr, Scale(1.0F));
    DrawText(graphics, "X", 10.5F, kMoonstone, CloseButtonBounds(), EAlign::Center,
             VALLEY_SPIRIT_FONT);

    DrawText(graphics, "THE PHILOSOPHY", 11.5F, kJade,
             IRECT(X(59.0F), Y(111.0F), X(260.0F), Y(133.0F)));
    DrawText(graphics,
             "In Daoist thought, the valley is empty yet responsive. It receives energy without holding it.",
             12.1F, kMoonstone, IRECT(X(59.0F), Y(133.0F), X(695.0F), Y(154.0F)));
    DrawText(graphics,
             "Valley Spirit receives a sound, lets it travel, and returns it transformed.",
             11.5F, kMuted, IRECT(X(59.0F), Y(155.0F), X(695.0F), Y(176.0F)));

    DrawText(graphics, "THE TECHNIQUE", 11.5F, kJade,
             IRECT(X(59.0F), Y(188.0F), X(320.0F), Y(210.0F)));
    DrawControl(graphics, 212.0F, "ECHO TIME", "Distance before the first return", 59.0F);
    DrawControl(graphics, 248.0F, "PITCH", "Raises or lowers every spectral return", 59.0F);
    DrawControl(graphics, 284.0F, "FEEDBACK", "Length and persistence of the echo path", 59.0F);
    DrawControl(graphics, 212.0F, "DIFFUSION", "Softens each return into a wider cloud", 382.0F);
    DrawControl(graphics, 266.0F, "MIX", "Blends the transformed path with the source", 382.0F);

    graphics.FillRect(IColor(255, 66, 65, 61), IRECT(X(59.0F), Y(323.0F), X(695.0F), Y(324.0F)));
    DrawText(graphics, "RITUAL CONTROLS", 10.8F, kBronze,
             IRECT(X(59.0F), Y(334.0F), X(220.0F), Y(354.0F)));
    DrawText(graphics, "SYNC follows the host division. FREEZE holds the current valley without new input.",
             11.0F, kMoonstone, IRECT(X(59.0F), Y(354.0F), X(695.0F), Y(374.0F)));
    DrawText(graphics, "PATH: Reflection keeps pitch stable; Spiral compounds pitch; Cloud compounds pitch and diffusion.",
             10.7F, kMuted, IRECT(X(59.0F), Y(376.0F), X(695.0F), Y(397.0F)));
    DrawText(graphics, "Start with Cloud, +7 st and moderate Feedback; use Freeze as a playable gesture.",
             10.5F, kMuted, IRECT(X(59.0F), Y(398.0F), X(695.0F), Y(419.0F)));
  }

  void DrawControl(IGraphics& graphics, float y, const char* name, const char* description,
                   float x) const {
    DrawText(graphics, name, 11.5F, kBronze,
             IRECT(X(x), Y(y), X(x + 285.0F), Y(y + 18.0F)), EAlign::Near,
             VALLEY_SPIRIT_FONT);
    DrawText(graphics, description, 10.8F, kMuted,
             IRECT(X(x), Y(y + 17.0F), X(x + 285.0F), Y(y + 35.0F)));
  }

  void DrawText(IGraphics& graphics, const char* text, float size, const IColor& color,
                const IRECT& bounds, EAlign align = EAlign::Near,
                const char* font = DEFAULT_FONT) const {
    graphics.DrawText(IText(Scale(size), color, font, align, EVAlign::Middle), text, bounds);
  }

  float X(float coordinate) const { return mRECT.L + coordinate * (mRECT.W() / 760.0F); }
  float Y(float coordinate) const { return mRECT.T + coordinate * (mRECT.H() / 460.0F); }
  float Scale(float amount) const {
    return amount * std::min(mRECT.W() / 760.0F, mRECT.H() / 460.0F);
  }

  bool isOpen_ = false;
};
