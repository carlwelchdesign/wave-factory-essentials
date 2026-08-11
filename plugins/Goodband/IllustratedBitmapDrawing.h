#pragma once

#include <algorithm>

#include "IControls.h"

namespace threefold {

inline IRECT AspectFitBounds(const IBitmap& bitmap, const IRECT& bounds) {
  const auto bitmapAspect = static_cast<float>(bitmap.W()) / static_cast<float>(bitmap.H());
  const auto boundsAspect = bounds.W() / bounds.H();

  if (boundsAspect > bitmapAspect) {
    const auto width = bounds.H() * bitmapAspect;
    return IRECT(bounds.MW() - width * 0.5F, bounds.T, bounds.MW() + width * 0.5F, bounds.B);
  }

  const auto height = bounds.W() / bitmapAspect;
  return IRECT(bounds.L, bounds.MH() - height * 0.5F, bounds.R, bounds.MH() + height * 0.5F);
}

inline void DrawBitmapAspectFit(IGraphics& graphics, const IBitmap& bitmap, const IRECT& bounds,
                                const IBlend* blend = nullptr) {
  graphics.DrawFittedBitmap(bitmap, AspectFitBounds(bitmap, bounds), blend);
}

inline void DrawBitmapRegionFitted(IGraphics& graphics, const IBitmap& bitmap, const IRECT& source,
                                   const IRECT& destination, const IBlend* blend = nullptr) {
  graphics.PathTransformSave();
  graphics.PathTransformTranslate(destination.L, destination.T);
  graphics.PathTransformScale(destination.W() / source.W(), destination.H() / source.H());
  graphics.DrawBitmap(bitmap, IRECT(0.0F, 0.0F, source.W(), source.H()), static_cast<int>(source.L),
                      static_cast<int>(source.T), blend);
  graphics.PathTransformRestore();
}

inline void DrawThreeSlicePlate(IGraphics& graphics, const IBitmap& bitmap, const IRECT& bounds,
                                const IBlend* blend = nullptr) {
  const auto sourceWidth = static_cast<float>(bitmap.W());
  const auto sourceHeight = static_cast<float>(bitmap.H());
  const auto sourceCap = std::min(sourceHeight * 0.48F, sourceWidth * 0.22F);
  const auto destinationCap = std::min(bounds.H() * 0.48F, bounds.W() * 0.28F);

  DrawBitmapRegionFitted(graphics, bitmap, IRECT(0.0F, 0.0F, sourceCap, sourceHeight),
                         IRECT(bounds.L, bounds.T, bounds.L + destinationCap, bounds.B), blend);
  DrawBitmapRegionFitted(graphics, bitmap,
                         IRECT(sourceCap, 0.0F, sourceWidth - sourceCap, sourceHeight),
                         IRECT(bounds.L + destinationCap, bounds.T, bounds.R - destinationCap, bounds.B),
                         blend);
  DrawBitmapRegionFitted(graphics, bitmap,
                         IRECT(sourceWidth - sourceCap, 0.0F, sourceWidth, sourceHeight),
                         IRECT(bounds.R - destinationCap, bounds.T, bounds.R, bounds.B), blend);
}

class AspectFitBitmapControl final : public IControl {
public:
  AspectFitBitmapControl(const IRECT& bounds, const IBitmap& bitmap)
      : IControl(bounds), bitmap_(bitmap) {
    SetIgnoreMouse(true);
  }

  void Draw(IGraphics& graphics) override { DrawBitmapAspectFit(graphics, bitmap_, mRECT); }

  void OnRescale() override { bitmap_ = GetUI()->GetScaledBitmap(bitmap_); }

private:
  IBitmap bitmap_;
};

}  // namespace threefold
