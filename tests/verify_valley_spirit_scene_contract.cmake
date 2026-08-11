set(plugin_root "${PROJECT_ROOT}/plugins/PitchTrails")
set(scene_header "${plugin_root}/ValleySpiritSceneControl.h")
set(help_header "${plugin_root}/ValleySpiritHelpControl.h")
set(dial_header "${plugin_root}/IllustratedSpiritDialControl.h")
set(plugin_source "${plugin_root}/PitchTrails.cpp")
set(plugin_config "${plugin_root}/config.h")
set(plugin_cmake "${plugin_root}/CMakeLists.txt")
set(windows_resources "${plugin_root}/resources/main.rc")

function(verify_png_dimensions asset_name expected_width expected_height)
  set(asset_path "${plugin_root}/resources/img/${asset_name}")
  if(NOT EXISTS "${asset_path}")
    message(FATAL_ERROR "Valley Spirit scene asset is missing: ${asset_name}")
  endif()
  file(READ "${asset_path}" png_header HEX LIMIT 24)
  string(SUBSTRING "${png_header}" 32 8 width_hex)
  string(SUBSTRING "${png_header}" 40 8 height_hex)
  math(EXPR actual_width "0x${width_hex}")
  math(EXPR actual_height "0x${height_hex}")
  if(NOT actual_width EQUAL expected_width OR NOT actual_height EQUAL expected_height)
    message(FATAL_ERROR
      "${asset_name} must be ${expected_width}x${expected_height}, got ${actual_width}x${actual_height}")
  endif()
endfunction()

verify_png_dimensions("valley-spirit-bg.png" 760 460)
verify_png_dimensions("valley-spirit-bg@2x.png" 1520 920)
verify_png_dimensions("valley-spirit-frame.png" 760 460)
verify_png_dimensions("valley-spirit-frame@2x.png" 1520 920)
verify_png_dimensions("moonstone-dial.png" 256 256)
verify_png_dimensions("moonstone-dial@2x.png" 512 512)
verify_png_dimensions("jade-dial.png" 192 192)
verify_png_dimensions("jade-dial@2x.png" 384 384)

if(NOT EXISTS "${plugin_root}/resources/fonts/Marcellus-Regular.ttf")
  message(FATAL_ERROR "Valley Spirit must bundle its title and control font")
endif()

file(READ "${scene_header}" scene_contents)
foreach(required_scene_token
    "IControl(bounds, {delayParam, pitchParam, feedbackParam, diffusionParam, mixParam})"
    "SetIgnoreMouse(true)"
    "SetValueFromDelegate"
    "SetAnimation"
    "DrawReturningRibbons"
    "DrawEchoFragments"
    "PathCubicBezierTo"
    "FillTriangle"
    "kMaximumFragments = 52")
  string(FIND "${scene_contents}" "${required_scene_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit scene must include ${required_scene_token}")
  endif()
endforeach()

file(READ "${dial_header}" dial_contents)
foreach(required_dial_token
    "class IllustratedSpiritDialControl"
    "DrawFittedBitmap"
    "DrawNeedle"
    "DrawSettingEnergy"
    "GetScaledBitmap")
  string(FIND "${dial_contents}" "${required_dial_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit dial must include ${required_dial_token}")
  endif()
endforeach()

file(READ "${help_header}" help_contents)
foreach(required_help_token
    "THE VALLEY SPIRIT MANUAL"
    "PITCH-SHIFTING DELAY / RETURNING ENERGY"
    "the valley is empty yet responsive"
    "returns it transformed"
    "ECHO TIME"
    "PITCH"
    "FEEDBACK"
    "DIFFUSION"
    "MIX"
    "QUICK START"
    "HelpButtonBounds"
    "CloseButtonBounds")
  string(FIND "${help_contents}" "${required_help_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit help panel must include ${required_help_token}")
  endif()
endforeach()

file(READ "${plugin_source}" source_contents)
foreach(required_source_token
    "LoadFont(VALLEY_SPIRIT_FONT, VALLEY_SPIRIT_FONT_FN)"
    "LoadBitmap(VALLEY_SPIRIT_BG_FN)"
    "LoadBitmap(VALLEY_SPIRIT_FRAME_FN)"
    "LoadBitmap(VALLEY_SPIRIT_MOONSTONE_DIAL_FN)"
    "LoadBitmap(VALLEY_SPIRIT_JADE_DIAL_FN)"
    "VALLEY SPIRIT"
    "THE VALLEY ANSWERS WITHOUT END"
    "new ValleySpiritSceneControl"
    "new IllustratedSpiritDialControl"
    "new ValleySpiritHelpControl")
  string(FIND "${source_contents}" "${required_source_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit plugin must include ${required_source_token}")
  endif()
endforeach()

string(FIND "${source_contents}" "new ValleySpiritBitmapControl(bounds, background)" background_position)
string(FIND "${source_contents}" "new ValleySpiritSceneControl" scene_position)
string(FIND "${source_contents}" "new IllustratedSpiritDialControl" dial_position)
string(FIND "${source_contents}" "new ValleySpiritBitmapControl(bounds, frame)" frame_position)
string(FIND "${source_contents}" "new ValleySpiritHelpControl" help_position)
if(background_position EQUAL -1 OR scene_position EQUAL -1 OR dial_position EQUAL -1 OR
   frame_position EQUAL -1 OR help_position EQUAL -1 OR
   NOT background_position LESS scene_position OR NOT scene_position LESS dial_position OR
   NOT dial_position LESS frame_position OR NOT frame_position LESS help_position)
  message(FATAL_ERROR "Valley Spirit scene layering order is invalid")
endif()

file(READ "${plugin_config}" config_contents)
foreach(required_config_token
    "#define PLUG_NAME \"Valley Spirit\""
    "#define PLUG_VERSION_STR \"0.1.3\""
    "#define PLUG_UNIQUE_ID 'WfPt'"
    "#define BUNDLE_NAME \"PitchTrails\""
    "#define VALLEY_SPIRIT_FONT_FN \"Marcellus-Regular.ttf\"")
  string(FIND "${config_contents}" "${required_config_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit config must include ${required_config_token}")
  endif()
endforeach()

file(READ "${plugin_cmake}" cmake_contents)
foreach(required_cmake_token
    "IllustratedSpiritDialControl.h"
    "ValleySpiritHelpControl.h"
    "ValleySpiritSceneControl.h"
    "resources/fonts/Marcellus-Regular.ttf"
    "resources/img/valley-spirit-bg@2x.png"
    "resources/img/valley-spirit-frame@2x.png"
    "resources/img/moonstone-dial@2x.png"
    "resources/img/jade-dial@2x.png"
    "if(WIN32)"
    "resources/main.rc"
    "PitchTrails-vst3"
    "PitchTrails-clap")
  string(FIND "${cmake_contents}" "${required_cmake_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit CMake setup must include ${required_cmake_token}")
  endif()
endforeach()

file(READ "${windows_resources}" windows_resource_contents)
foreach(required_windows_token
    "VALLEY_SPIRIT_FONT_FN TTF"
    "VALLEY_SPIRIT_BG_FN PNG"
    "valley-spirit-bg@2x.png"
    "VALLEY_SPIRIT_FRAME_FN PNG"
    "VALLEY_SPIRIT_MOONSTONE_DIAL_FN PNG"
    "VALLEY_SPIRIT_JADE_DIAL_FN PNG")
  string(FIND "${windows_resource_contents}" "${required_windows_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit Windows resources must include ${required_windows_token}")
  endif()
endforeach()

foreach(format_name AU VST3 CLAP)
  file(READ "${plugin_root}/resources/PitchTrails-${format_name}-Info.plist" plist_contents)
  foreach(required_plist_token
      "<string>Valley Spirit</string>"
      "<string>PitchTrails</string>"
      "<string>0.1.3</string>")
    string(FIND "${plist_contents}" "${required_plist_token}" token_position)
    if(token_position EQUAL -1)
      message(FATAL_ERROR "Valley Spirit ${format_name} metadata must include ${required_plist_token}")
    endif()
  endforeach()
endforeach()
