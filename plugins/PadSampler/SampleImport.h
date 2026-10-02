#pragma once
#include <string>

namespace padsampler {
struct ImportedSample {
  std::string path;
  std::string name;
  std::string error;
  bool cancelled = false;
};

// Called on the editor thread while the open panel or drag pasteboard still owns
// the sandbox extension. The returned path is an immutable, app-owned copy.
ImportedSample ChooseImportedSample();
ImportedSample ImportDroppedSample(const std::string& droppedPath);
}
