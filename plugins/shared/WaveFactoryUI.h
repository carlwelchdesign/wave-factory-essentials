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

inline iplug::igraphics::IVStyle MakeCinematicControlStyle(const iplug::igraphics::IColor& accent) {
  using namespace iplug::igraphics;

  return DEFAULT_STYLE.WithShowLabel(true)
      .WithShowValue(true)
      .WithLabelText(IText(12.0F, IColor(255, 226, 220, 204)))
      .WithValueText(IText(12.0F, IColor(255, 192, 164, 104)))
      .WithColor(kBG, IColor(182, 8, 11, 12))
      .WithColor(kFG, IColor(255, 74, 72, 66))
      .WithColor(kPR, accent)
      .WithColor(kFR, IColor(255, 177, 139, 70))
      .WithColor(kX1, IColor(255, 238, 232, 216))
      .WithDrawShadows(false)
      .WithDrawFrame(false)
      .WithWidgetFrac(0.72F);
}

}  // namespace wfe::ui
