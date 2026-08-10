#pragma once

#include <algorithm>

#include "IControls.h"

namespace threefold::vfx {

inline void DrawAtlasSprite(IGraphics& graphics, const IBitmap& atlas, int spriteIndex, float centerX,
                            float centerY, float size, float rotationDegrees, float opacity) {
  constexpr int kAtlasColumns = 4;
  constexpr int kAtlasRows = 4;
  const auto cellWidth = static_cast<float>(atlas.W()) / static_cast<float>(kAtlasColumns);
  const auto cellHeight = static_cast<float>(atlas.H()) / static_cast<float>(kAtlasRows);
  const auto clampedIndex = std::clamp(spriteIndex, 0, kAtlasColumns * kAtlasRows - 1);
  const auto column = clampedIndex % kAtlasColumns;
  const auto row = clampedIndex / kAtlasColumns;
  const auto source = IRECT(column * cellWidth, row * cellHeight, (column + 1) * cellWidth,
                            (row + 1) * cellHeight);

  graphics.PathTransformSave();
  graphics.PathTransformTranslate(centerX, centerY);
  graphics.PathTransformRotate(rotationDegrees);
  graphics.PathTransformScale(size / source.W(), size / source.H());
  IBlend spriteBlend(EBlend::Add, std::clamp(opacity, 0.0F, 1.0F));
  graphics.DrawBitmap(atlas, IRECT(-source.W() * 0.5F, -source.H() * 0.5F, source.W() * 0.5F,
                                   source.H() * 0.5F),
                      static_cast<int>(source.L), static_cast<int>(source.T), &spriteBlend);
  graphics.PathTransformRestore();
}

inline void DrawChiRibbon(IGraphics& graphics, float startX, float startY, float endX, float endY,
                          float bend, const IColor& color, float intensity, float baseWidth) {
  const auto deltaX = endX - startX;
  const auto deltaY = endY - startY;
  const auto control1X = startX + deltaX * 0.28F;
  const auto control1Y = startY + deltaY * 0.18F - bend;
  const auto control2X = startX + deltaX * 0.70F;
  const auto control2Y = startY + deltaY * 0.82F + bend * 0.72F;

  constexpr float kWidths[]{4.6F, 2.1F, 0.72F};
  constexpr float kOpacities[]{0.075F, 0.22F, 0.78F};
  for (int layer = 0; layer < 3; ++layer) {
    graphics.PathClear();
    graphics.PathMoveTo(startX, startY);
    graphics.PathCubicBezierTo(control1X, control1Y, control2X, control2Y, endX, endY);
    IBlend blend(EBlend::Add, std::clamp(intensity * kOpacities[layer], 0.0F, 1.0F));
    graphics.PathStroke(IPattern(color), baseWidth * kWidths[layer], IStrokeOptions(), &blend);
  }
}

}  // namespace threefold::vfx
