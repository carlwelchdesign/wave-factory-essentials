#pragma once

#include "IControls.h"

namespace wfe::ui {

inline iplug::igraphics::IVStyle MakeDarkControlStyle(const iplug::igraphics::IColor& accent) {
  using namespace iplug::igraphics;

  return DEFAULT_STYLE.WithShowLabel(true)
      .WithShowValue(true)
      .WithLabelText(IText(15.0F, IColor(255, 232, 238, 235)))
      .WithValueText(IText(13.0F, IColor(255, 190, 205, 200)))
      .WithColor(kBG, IColor(255, 31, 38, 37))
      .WithColor(kFG, IColor(255, 72, 84, 80))
      .WithColor(kPR, accent)
      .WithColor(kFR, IColor(255, 125, 140, 134))
      .WithColor(kX1, IColor(255, 224, 232, 228))
      .WithDrawShadows(false)
      .WithWidgetFrac(0.78F);
}

}  // namespace wfe::ui
