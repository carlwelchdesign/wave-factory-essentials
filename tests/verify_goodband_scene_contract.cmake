set(goodband_root "${PROJECT_ROOT}/plugins/Goodband")
set(scene_header "${goodband_root}/GoodbandSceneControl.h")
set(plugin_source "${goodband_root}/Goodband.cpp")
set(plugin_cmake "${goodband_root}/CMakeLists.txt")

foreach(asset_name "arcade-dojo-bg.png" "arcade-dojo-bg@2x.png")
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
    "DrawEnergyTrail"
    "DrawCharacterGesture")
  string(FIND "${scene_contents}" "${required_scene_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Goodband scene must include ${required_scene_token}")
  endif()
endforeach()

file(READ "${plugin_source}" source_contents)
string(FIND "${source_contents}" "LoadBitmap(ARCADE_DOJO_BG_FN)" bitmap_position)
string(FIND "${source_contents}" "new GoodbandSceneControl" scene_position)
string(FIND "${source_contents}" "new IVKnobControl" knob_position)
if(bitmap_position EQUAL -1 OR scene_position EQUAL -1 OR knob_position EQUAL -1)
  message(FATAL_ERROR "Goodband must load and attach its scene before parameter controls")
endif()
if(NOT bitmap_position LESS scene_position OR NOT scene_position LESS knob_position)
  message(FATAL_ERROR "Goodband scene layering order is invalid")
endif()

file(READ "${plugin_cmake}" cmake_contents)
foreach(asset_name "arcade-dojo-bg.png" "arcade-dojo-bg@2x.png")
  string(FIND "${cmake_contents}" "resources/img/${asset_name}" asset_position)
  if(asset_position EQUAL -1)
    message(FATAL_ERROR "Goodband CMake resources must include ${asset_name}")
  endif()
endforeach()
