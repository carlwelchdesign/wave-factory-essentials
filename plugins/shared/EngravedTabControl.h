#pragma once

#include <algorithm>

#include "IControls.h"

namespace wfe::ui {

class EngravedTabControl final : public iplug::igraphics::IVTabSwitchControl {
public:
  EngravedTabControl(const iplug::igraphics::IRECT& bounds, int paramIndex,
                     const std::vector<const char*>& options, const char* font,
                     const iplug::igraphics::IColor& accent)
      : IVTabSwitchControl(bounds, paramIndex, options, "", iplug::igraphics::DEFAULT_STYLE,
                           iplug::igraphics::EVShape::Rectangle,
                           iplug::igraphics::EDirection::Horizontal),
        font_(font), accent_(accent) {}

  void Draw(iplug::igraphics::IGraphics& graphics) override { DrawWidget(graphics); }

  void DrawWidget(iplug::igraphics::IGraphics& graphics) override {
    using namespace iplug::igraphics;
    const auto selected = GetSelectedIdx();
    for (int index = 0; index < mNumStates; ++index) {
      const auto bounds = mButtons.Get()[index].GetPadded(-1.5F);
      const auto active = index == selected;
      const auto hovered = index == mMouseOverButton;
      graphics.FillRect(active ? IColor(224, 48, 38, 22) : IColor(190, 7, 10, 13), bounds);
      if (active) {
        IBlend glow(EBlend::Add, 0.25F);
        graphics.FillRect(accent_.WithOpacity(0.20F), bounds.GetPadded(2.0F), &glow);
      }
      graphics.DrawRect(active ? accent_ : IColor(220, 111, 87, 52), bounds, nullptr,
                        active ? 1.2F : 0.8F);
      graphics.DrawRect(IColor(150, 28, 27, 24), bounds.GetPadded(-2.0F), nullptr, 0.6F);
      if (mTabLabels.Get(index)) {
        graphics.DrawText(IText(active ? 9.5F : 9.0F,
                                active ? IColor(255, 244, 225, 184)
                                       : (hovered ? IColor(255, 220, 209, 188)
                                                  : IColor(255, 166, 158, 145)),
                                font_, EAlign::Center, EVAlign::Middle),
                          mTabLabels.Get(index)->Get(), bounds);
      }
    }
  }

private:
  const char* font_ = DEFAULT_FONT;
  iplug::igraphics::IColor accent_;
};

}  // namespace wfe::ui
