#pragma once

#include "IControls.h"

class ValleySpiritBitmapControl final : public IControl {
public:
  ValleySpiritBitmapControl(const IRECT& bounds, const IBitmap& bitmap)
      : IControl(bounds), bitmap_(bitmap) {
    SetIgnoreMouse(true);
  }

  void Draw(IGraphics& graphics) override { graphics.DrawFittedBitmap(bitmap_, mRECT); }

  void OnRescale() override { bitmap_ = GetUI()->GetScaledBitmap(bitmap_); }

private:
  IBitmap bitmap_;
};
