set(goodband_root "${PROJECT_ROOT}/plugins/Goodband")
set(scene_header "${goodband_root}/GoodbandSceneControl.h")
set(backdrop_header "${goodband_root}/GoodbandBackdropControl.h")
set(shuriken_header "${goodband_root}/ShurikenKnobControl.h")
set(plugin_source "${goodband_root}/Goodband.cpp")
set(plugin_cmake "${goodband_root}/CMakeLists.txt")

foreach(asset_name
    "temple-of-mastery-bg.png"
    "temple-of-mastery-bg@2x.png"
    "temple-of-mastery-gesture-bg.png"
    "temple-of-mastery-gesture-bg@2x.png")
  if(NOT EXISTS "${goodband_root}/resources/img/${asset_name}")
    message(FATAL_ERROR "Goodband scene asset is missing: ${asset_name}")
  endif()
endforeach()

file(READ "${scene_header}" scene_contents)
foreach(required_scene_token
    "IControl(bounds, {amountParam, characterParam, mixParam, outputParam})"
    "SetIgnoreMouse(true)"
    "SetValueFromDelegate"
    "SetAnimation"
    "DrawRestingConstellation"
    "DrawMagicParticles"
    "PositionParticle"
    "kMaximumAnimatedParticles = 84")
  string(FIND "${scene_contents}" "${required_scene_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Goodband scene must include ${required_scene_token}")
  endif()
endforeach()

string(FIND "${scene_contents}" "DrawLine" line_position)
if(NOT line_position EQUAL -1)
  message(FATAL_ERROR "Goodband magical animation must use particles, not line effects")
endif()

file(READ "${backdrop_header}" backdrop_contents)
foreach(required_backdrop_token
    "kGestureCycleDurationMs = 60000"
    "kGestureParticleCount = 72"
    "gestureBackground_"
    "DrawGestureEnergy"
    "OnEndAnimation")
  string(FIND "${backdrop_contents}" "${required_backdrop_token}" backdrop_position)
  if(backdrop_position EQUAL -1)
    message(FATAL_ERROR "Goodband backdrop must include ${required_backdrop_token}")
  endif()
endforeach()

file(READ "${shuriken_header}" shuriken_contents)
foreach(required_shuriken_token
    "class ShurikenKnobControl"
    "BuildShurikenPath"
    "DrawIndicatorTrack"
    "PathFill"
    "PathStroke")
  string(FIND "${shuriken_contents}" "${required_shuriken_token}" shuriken_position)
  if(shuriken_position EQUAL -1)
    message(FATAL_ERROR "Goodband shuriken control must include ${required_shuriken_token}")
  endif()
endforeach()

file(READ "${plugin_source}" source_contents)
string(FIND "${source_contents}" "LoadBitmap(TEMPLE_OF_MASTERY_BG_FN)" bitmap_position)
string(FIND "${source_contents}" "LoadBitmap(TEMPLE_OF_MASTERY_GESTURE_BG_FN)" gesture_bitmap_position)
string(FIND "${source_contents}" "new GoodbandBackdropControl" backdrop_position)
string(FIND "${source_contents}" "new GoodbandSceneControl" scene_position)
string(FIND "${source_contents}" "new ShurikenKnobControl" knob_position)
if(bitmap_position EQUAL -1 OR gesture_bitmap_position EQUAL -1 OR backdrop_position EQUAL -1 OR
   scene_position EQUAL -1 OR knob_position EQUAL -1)
  message(FATAL_ERROR "Goodband must load and attach its scene before parameter controls")
endif()
if(NOT bitmap_position LESS gesture_bitmap_position OR NOT gesture_bitmap_position LESS backdrop_position OR
   NOT backdrop_position LESS scene_position OR NOT scene_position LESS knob_position)
  message(FATAL_ERROR "Goodband scene layering order is invalid")
endif()

file(READ "${plugin_cmake}" cmake_contents)
foreach(resource_path
    "resources/img/temple-of-mastery-bg.png"
    "resources/img/temple-of-mastery-bg@2x.png"
    "resources/img/temple-of-mastery-gesture-bg.png"
    "resources/img/temple-of-mastery-gesture-bg@2x.png"
    "resources/fonts/MaShanZheng-Regular.ttf"
    "resources/fonts/MaShanZheng-OFL.txt")
  string(FIND "${cmake_contents}" "${resource_path}" asset_position)
  if(asset_position EQUAL -1)
    message(FATAL_ERROR "Goodband CMake resources must include ${resource_path}")
  endif()
endforeach()

foreach(required_preset_token
    "IVTabSwitchControl"
    "ApplyCharacterPreset"
    "GetGoodbandCharacterPreset"
    "BeginInformHostOfParamChangeFromUI"
    "SendParameterValueFromUI"
    "ForControlWithParam")
  string(FIND "${source_contents}" "${required_preset_token}" preset_position)
  if(preset_position EQUAL -1)
    message(FATAL_ERROR "Goodband Character selector must include ${required_preset_token}")
  endif()
endforeach()
