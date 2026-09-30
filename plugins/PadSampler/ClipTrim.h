#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace padsampler {
// End is exclusive; null means the end of the original, unmodified sample.
struct ClipTrim {
  double start = 0.;
  std::optional<double> end;
  bool operator==(const ClipTrim& other) const { return start == other.start && end == other.end; }
  bool Valid() const {
    return std::isfinite(start) && start >= 0. && start <= 60. &&
      (!end || (std::isfinite(*end) && *end > start && *end <= 60.));
  }
};
struct ClipFrames { uint32_t start = 0, end = 0; bool reset = false; };
inline ClipFrames ResolveTrim(const ClipTrim& trim, double rate, size_t frames) {
  if (!trim.Valid() || !std::isfinite(rate) || rate <= 0. || frames > UINT32_MAX) throw std::runtime_error("Invalid clip trim or sample length");
  auto start = uint64_t(std::llround(trim.start * rate));
  auto end = trim.end ? uint64_t(std::llround(*trim.end * rate)) : frames;
  if (start >= frames || frames - start < 2 || end > frames || end <= start || end - start < 2)
    return {0, uint32_t(frames), true};
  return {uint32_t(start), uint32_t(end), false};
}
inline ClipTrim FramesToTrim(uint32_t start, uint32_t end, double rate, size_t frames) {
  return {double(start) / rate, end == frames ? std::optional<double>{} : std::optional<double>{double(end) / rate}};
}
struct ClipHistory {
  std::array<ClipTrim, 32> undo{}, redo{};
  int nUndo = 0, nRedo = 0;
  void Commit(ClipTrim before, ClipTrim after) {
    if (before == after) return;
    if (nUndo == 32) { std::move(undo.begin() + 1, undo.end(), undo.begin()); --nUndo; }
    undo[nUndo++] = before; nRedo = 0;
  }
  bool Undo(ClipTrim& current) {
    if (!nUndo) return false;
    redo[nRedo++] = current; current = undo[--nUndo]; return true;
  }
  bool Redo(ClipTrim& current) {
    if (!nRedo) return false;
    undo[nUndo++] = current; current = redo[--nRedo]; return true;
  }
};
}
