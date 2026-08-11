set(public_files
  "README.md"
  "LICENSE-NOTICE.md"
  "docs/product-brief.md"
  "docs/roadmap.md"
  "packaging/README-Windows.txt"
  "packaging/README-Valley-Spirit-Windows.txt"
  "packaging/README-Threefold-Palm-macOS.txt"
  "packaging/README-Valley-Spirit-macOS.txt"
  "packaging/Install-Threefold-Palm.command"
  "packaging/Install-Valley-Spirit.command"
  "packaging/Install-Threefold-Palm.ps1"
  "scripts/package-macos.sh"
  ".github/workflows/tester-builds.yml"
  "plugins/Goodband/Goodband.cpp"
  "plugins/Goodband/GoodbandHelpControl.h"
  "plugins/Goodband/config.h"
  "plugins/Goodband/resources/Goodband-AU-Info.plist"
  "plugins/Goodband/resources/img/README.md"
  "plugins/PitchTrails/PitchTrails.cpp"
  "plugins/PitchTrails/ValleySpiritHelpControl.h"
  "plugins/PitchTrails/config.h"
  "plugins/PitchTrails/resources/PitchTrails-AU-Info.plist")

foreach(relative_path IN LISTS public_files)
  set(file_path "${PROJECT_ROOT}/${relative_path}")
  if(NOT EXISTS "${file_path}")
    message(FATAL_ERROR "Public brand contract file is missing: ${relative_path}")
  endif()

  file(READ "${file_path}" contents)
  foreach(retired_brand "Wave Factory" "Wave-Factory-Essentials")
    string(FIND "${contents}" "${retired_brand}" brand_position)
    if(NOT brand_position EQUAL -1)
      message(FATAL_ERROR "Public-facing file ${relative_path} still contains ${retired_brand}")
    endif()
  endforeach()
endforeach()
