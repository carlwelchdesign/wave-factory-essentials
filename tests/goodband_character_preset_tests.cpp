#include <cassert>

#include "plugins/Goodband/GoodbandCharacterPresets.h"

int main() {
  using wfe::ui::GetGoodbandCharacterPreset;

  const auto clean = GetGoodbandCharacterPreset(0);
  assert(clean.amountPercent == 22.0);
  assert(clean.mixPercent == 72.0);
  assert(clean.outputTrimDb == 0.0);

  const auto warm = GetGoodbandCharacterPreset(1);
  assert(warm.amountPercent == 48.0);
  assert(warm.mixPercent == 82.0);
  assert(warm.outputTrimDb == -0.8);

  const auto punch = GetGoodbandCharacterPreset(2);
  assert(punch.amountPercent == 62.0);
  assert(punch.mixPercent == 88.0);
  assert(punch.outputTrimDb == -1.5);

  const auto wide = GetGoodbandCharacterPreset(3);
  assert(wide.amountPercent == 44.0);
  assert(wide.mixPercent == 78.0);
  assert(wide.outputTrimDb == -1.0);

  assert(&GetGoodbandCharacterPreset(-1) == &wfe::ui::kGoodbandCharacterPresets.front());
  assert(&GetGoodbandCharacterPreset(99) == &wfe::ui::kGoodbandCharacterPresets.back());
}
