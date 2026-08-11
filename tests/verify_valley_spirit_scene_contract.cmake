set(plugin_root "${PROJECT_ROOT}/plugins/PitchTrails")
set(scene_header "${plugin_root}/ValleySpiritSceneControl.h")
set(backdrop_header "${plugin_root}/ValleySpiritBackdropControl.h")
set(vfx_renderer_header "${plugin_root}/ValleySpiritVfxRenderer.h")
set(help_header "${plugin_root}/ValleySpiritHelpControl.h")
set(dial_header "${plugin_root}/IllustratedSpiritDialControl.h")
set(plugin_source "${plugin_root}/PitchTrails.cpp")
set(plugin_config "${plugin_root}/config.h")
set(plugin_cmake "${plugin_root}/CMakeLists.txt")
set(windows_resources "${plugin_root}/resources/main.rc")
set(windows_packager "${PROJECT_ROOT}/scripts/package-windows.ps1")
set(windows_installer "${PROJECT_ROOT}/packaging/Install-Valley-Spirit.ps1")
set(windows_readme "${PROJECT_ROOT}/packaging/README-Valley-Spirit-Windows.txt")
set(tester_workflow "${PROJECT_ROOT}/.github/workflows/tester-builds.yml")

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
verify_png_dimensions("valley-spirit-gesture-bg.png" 760 460)
verify_png_dimensions("valley-spirit-gesture-bg@2x.png" 1520 920)
verify_png_dimensions("valley-spirit-frame.png" 760 460)
verify_png_dimensions("valley-spirit-frame@2x.png" 1520 920)
verify_png_dimensions("moonstone-dial.png" 256 256)
verify_png_dimensions("moonstone-dial@2x.png" 512 512)
verify_png_dimensions("jade-dial.png" 192 192)
verify_png_dimensions("jade-dial@2x.png" 384 384)
verify_png_dimensions("spirit-vfx-atlas.png" 512 512)
verify_png_dimensions("spirit-vfx-atlas@2x.png" 1024 1024)

if(NOT EXISTS "${plugin_root}/resources/fonts/Marcellus-Regular.ttf")
  message(FATAL_ERROR "Valley Spirit must bundle its title and control font")
endif()

file(READ "${scene_header}" scene_contents)
foreach(required_scene_token
    "IControl(bounds, {delayParam, pitchParam, feedbackParam, diffusionParam, mixParam,"
    "freezeParam, pathParam})"
    "SetIgnoreMouse(true)"
    "SetValueFromDelegate"
    "SetAnimation"
    "DrawReturningRibbons"
    "DrawEchoFragments"
    "PathCubicBezierTo"
    "DrawAtlasSprite"
    "kMaximumFragments = 52")
  string(FIND "${scene_contents}" "${required_scene_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit scene must include ${required_scene_token}")
  endif()
endforeach()

string(FIND "${scene_contents}" "FillTriangle" scene_triangle_position)
string(FIND "${scene_contents}" "DrawLine" scene_line_position)
if(NOT scene_triangle_position EQUAL -1 OR NOT scene_line_position EQUAL -1)
  message(FATAL_ERROR "Valley Spirit ambient motion must use textured sprites, not geometric particles")
endif()

file(READ "${backdrop_header}" backdrop_contents)
foreach(required_backdrop_token
    "IControl(bounds, {delayParam, pitchParam, feedbackParam, diffusionParam, mixParam,"
    "freezeParam, pathParam})"
    "kGestureCycleDurationMs = 12000"
    "kGestureWindowStart = 0.62F"
    "kGestureParticleCount = 58"
    "GestureAmount"
    "DrawGestureEnergy"
    "gestureBackground_"
    "DrawAtlasSprite"
    "DrawSpiritRibbon"
    "SetValueFromDelegate"
    "OnEndAnimation")
  string(FIND "${backdrop_contents}" "${required_backdrop_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit Sensei gesture must include ${required_backdrop_token}")
  endif()
endforeach()

file(READ "${vfx_renderer_header}" vfx_contents)
foreach(required_vfx_token
    "kAtlasColumns = 4"
    "kAtlasRows = 4"
    "DrawAtlasSprite"
    "DrawSpiritRibbon"
    "PathCubicBezierTo"
    "drawLayer(size * 1.32F")
  string(FIND "${vfx_contents}" "${required_vfx_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit VFX renderer must include ${required_vfx_token}")
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
    "\"THE VALLEY SPIRIT MANUAL\", 22.0F"
    "PITCH-SHIFTING DELAY / RETURNING ENERGY"
    "the valley is empty yet responsive"
    "returns it transformed"
    "ECHO TIME"
    "PITCH"
    "FEEDBACK"
    "DIFFUSION"
    "MIX"
    "RITUAL CONTROLS"
    "SYNC follows the host division"
    "Reflection keeps pitch stable"
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
    "LoadBitmap(VALLEY_SPIRIT_GESTURE_BG_FN)"
    "LoadBitmap(VALLEY_SPIRIT_FRAME_FN)"
    "LoadBitmap(VALLEY_SPIRIT_MOONSTONE_DIAL_FN)"
    "LoadBitmap(VALLEY_SPIRIT_JADE_DIAL_FN)"
    "LoadBitmap(VALLEY_SPIRIT_VFX_ATLAS_FN)"
    "VALLEY SPIRIT"
    "THE VALLEY ANSWERS WITHOUT END"
    "IRECT(24.0F, 27.0F, 384.0F, 86.0F)"
    "IText(43.0F"
    "IRECT(24.0F, 82.0F, 384.0F, 106.0F)"
    "IText(10.5F"
    "EAlign::Center"
    "new ValleySpiritBackdropControl"
    "new ValleySpiritSceneControl"
    "new IllustratedSpiritDialControl"
    "new ValleySpiritHelpControl")
  string(FIND "${source_contents}" "${required_source_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit plugin must include ${required_source_token}")
  endif()
endforeach()

string(FIND "${source_contents}" "new ValleySpiritBackdropControl" background_position)
string(FIND "${source_contents}" "new ValleySpiritSceneControl" scene_position)
string(FIND "${source_contents}" "new IllustratedSpiritDialControl" dial_position)
string(FIND "${source_contents}" "bounds.GetScaledAboutCentre(kFrameOverscanScale)" frame_position)
string(FIND "${source_contents}" "new ValleySpiritHelpControl" help_position)
if(background_position EQUAL -1 OR scene_position EQUAL -1 OR dial_position EQUAL -1 OR
   frame_position EQUAL -1 OR help_position EQUAL -1 OR
   NOT background_position LESS scene_position OR NOT scene_position LESS dial_position OR
   NOT dial_position LESS frame_position OR NOT frame_position LESS help_position)
  message(FATAL_ERROR "Valley Spirit scene layering order is invalid")
endif()

foreach(required_frame_token
    "kFrameOverscanScale = 1.048F"
    "bounds.GetScaledAboutCentre(kFrameOverscanScale)")
  string(FIND "${source_contents}" "${required_frame_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit frame must reach the plug-in edges using ${required_frame_token}")
  endif()
endforeach()

file(READ "${plugin_config}" config_contents)
foreach(required_config_token
    "#define PLUG_NAME \"Valley Spirit\""
    "#define PLUG_VERSION_STR \"0.1.5\""
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
    "ValleySpiritBackdropControl.h"
    "ValleySpiritHelpControl.h"
    "ValleySpiritSceneControl.h"
    "resources/fonts/Marcellus-Regular.ttf"
    "resources/img/valley-spirit-bg@2x.png"
    "resources/img/valley-spirit-gesture-bg@2x.png"
    "resources/img/valley-spirit-frame@2x.png"
    "resources/img/moonstone-dial@2x.png"
    "resources/img/jade-dial@2x.png"
    "resources/img/spirit-vfx-atlas@2x.png"
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
    "VALLEY_SPIRIT_GESTURE_BG_FN PNG"
    "valley-spirit-gesture-bg@2x.png"
    "VALLEY_SPIRIT_FRAME_FN PNG"
    "VALLEY_SPIRIT_MOONSTONE_DIAL_FN PNG"
    "VALLEY_SPIRIT_JADE_DIAL_FN PNG"
    "VALLEY_SPIRIT_VFX_ATLAS_FN PNG"
    "spirit-vfx-atlas@2x.png")
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
      "<string>0.1.5</string>")
    string(FIND "${plist_contents}" "${required_plist_token}" token_position)
    if(token_position EQUAL -1)
      message(FATAL_ERROR "Valley Spirit ${format_name} metadata must include ${required_plist_token}")
    endif()
  endforeach()
endforeach()

foreach(required_file
    "${windows_packager}"
    "${windows_installer}"
    "${windows_readme}"
    "${tester_workflow}")
  if(NOT EXISTS "${required_file}")
    message(FATAL_ERROR "Valley Spirit Windows publication file is missing: ${required_file}")
  endif()
endforeach()

file(READ "${windows_packager}" packager_contents)
foreach(required_packager_token
    "plugins/PitchTrails/config.h"
    "Valley-Spirit-$valleySpiritVersion-Windows-x64"
    "out/PitchTrails.vst3"
    "out/PitchTrails.clap"
    "Install-Valley-Spirit.ps1"
    "README-Valley-Spirit-Windows.txt")
  string(FIND "${packager_contents}" "${required_packager_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit Windows packager must include ${required_packager_token}")
  endif()
endforeach()

file(READ "${windows_installer}" installer_contents)
foreach(required_installer_token
    "PitchTrails.vst3"
    "PitchTrails.clap"
    "Programs\\Common"
    "Installed Valley Spirit VST3 and CLAP test builds")
  string(FIND "${installer_contents}" "${required_installer_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Valley Spirit Windows installer must include ${required_installer_token}")
  endif()
endforeach()

file(READ "${tester_workflow}" workflow_contents)
foreach(required_workflow_token
    "PitchTrails-vst3"
    "PitchTrails-clap"
    "Threefold-Palm-and-Valley-Spirit-Windows-x64"
    "dist/Valley-Spirit-*-Windows-x64.zip")
  string(FIND "${workflow_contents}" "${required_workflow_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Windows tester workflow must include ${required_workflow_token}")
  endif()
endforeach()
