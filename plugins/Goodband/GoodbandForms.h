#pragma once

#include <array>
#include <cstddef>

namespace wfe::ui {

struct GoodbandForm {
  const char* name;
  const char* shortName;
  int character;
  double amountPercent;
  double mixPercent;
  double outputTrimDb;
  bool autoGainMatch;
};

inline constexpr std::array<GoodbandForm, 5> kGoodbandForms{{
    {"First Stance", "STANCE", 0, 22.0, 72.0, 0.0, true},
    {"Iron Center", "CENTER", 1, 48.0, 82.0, -0.8, true},
    {"Striking Drum", "STRIKE", 2, 62.0, 88.0, -1.5, true},
    {"Parallel Flame", "FLAME", 1, 78.0, 48.0, -1.0, true},
    {"Open Hand", "OPEN", 3, 44.0, 78.0, -1.0, true},
}};

inline constexpr const GoodbandForm& GetGoodbandForm(int formIndex) {
  if (formIndex < 0) {
    return kGoodbandForms.front();
  }
  if (formIndex >= static_cast<int>(kGoodbandForms.size())) {
    return kGoodbandForms.back();
  }
  return kGoodbandForms[static_cast<std::size_t>(formIndex)];
}

}  // namespace wfe::ui
