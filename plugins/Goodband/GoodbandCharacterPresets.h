#pragma once

#include <array>
#include <cstddef>

namespace wfe::ui {

struct GoodbandCharacterPreset {
  double amountPercent;
  double mixPercent;
  double outputTrimDb;
};

inline constexpr std::array<GoodbandCharacterPreset, 4> kGoodbandCharacterPresets{{
    {22.0, 72.0, 0.0},
    {48.0, 82.0, -0.8},
    {62.0, 88.0, -1.5},
    {44.0, 78.0, -1.0},
}};

inline constexpr const GoodbandCharacterPreset& GetGoodbandCharacterPreset(int characterIndex) {
  if (characterIndex < 0) {
    return kGoodbandCharacterPresets.front();
  }
  if (characterIndex >= static_cast<int>(kGoodbandCharacterPresets.size())) {
    return kGoodbandCharacterPresets.back();
  }
  return kGoodbandCharacterPresets[static_cast<std::size_t>(characterIndex)];
}

}  // namespace wfe::ui
