set(goodband_root "${PROJECT_ROOT}/plugins/Goodband")
set(scene_header "${goodband_root}/GoodbandSceneControl.h")
set(backdrop_header "${goodband_root}/GoodbandBackdropControl.h")
set(help_header "${goodband_root}/GoodbandHelpControl.h")
set(vfx_renderer_header "${goodband_root}/ChiVfxRenderer.h")
set(shuriken_header "${goodband_root}/IllustratedShurikenKnobControl.h")
set(character_header "${goodband_root}/FightingGameCharacterControl.h")
set(plugin_source "${goodband_root}/Goodband.cpp")
set(plugin_cmake "${goodband_root}/CMakeLists.txt")
set(windows_resources "${goodband_root}/resources/main.rc")

function(verify_png_dimensions asset_name expected_width expected_height)
  set(asset_path "${goodband_root}/resources/img/${asset_name}")
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

foreach(asset_name
    "temple-of-mastery-bg.png"
    "temple-of-mastery-bg@2x.png"
    "temple-of-mastery-gesture-bg.png"
    "temple-of-mastery-gesture-bg@2x.png"
    "character-selector-unselected.png"
    "character-selector-unselected@2x.png"
    "character-selector-selected.png"
    "character-selector-selected@2x.png"
    "throwing-star-knob.png"
    "throwing-star-knob@2x.png"
    "knob-arena.png"
    "knob-arena@2x.png"
    "threefold-palm-wordmark.png"
    "threefold-palm-wordmark@2x.png"
    "threefold-palm-frame.png"
    "threefold-palm-frame@2x.png"
    "chi-vfx-atlas.png"
    "chi-vfx-atlas@2x.png")
  if(NOT EXISTS "${goodband_root}/resources/img/${asset_name}")
    message(FATAL_ERROR "Goodband scene asset is missing: ${asset_name}")
  endif()
endforeach()

verify_png_dimensions("threefold-palm-frame.png" 720 440)
verify_png_dimensions("threefold-palm-frame@2x.png" 1440 880)
verify_png_dimensions("threefold-palm-wordmark.png" 375 79)
verify_png_dimensions("threefold-palm-wordmark@2x.png" 750 158)
verify_png_dimensions("character-selector-unselected.png" 142 41)
verify_png_dimensions("character-selector-unselected@2x.png" 284 82)
verify_png_dimensions("character-selector-selected.png" 141 41)
verify_png_dimensions("character-selector-selected@2x.png" 282 82)
verify_png_dimensions("knob-arena.png" 128 126)
verify_png_dimensions("knob-arena@2x.png" 256 252)
verify_png_dimensions("throwing-star-knob.png" 108 105)
verify_png_dimensions("throwing-star-knob@2x.png" 216 210)
verify_png_dimensions("chi-vfx-atlas.png" 512 512)
verify_png_dimensions("chi-vfx-atlas@2x.png" 1024 1024)

file(READ "${scene_header}" scene_contents)
foreach(required_scene_token
    "IControl(bounds, {amountParam, characterParam, mixParam, outputParam})"
    "SetIgnoreMouse(true)"
    "SetValueFromDelegate"
    "SetAnimation"
    "DrawRestingConstellation"
    "DrawMagicParticles"
    "DrawChiRibbons"
    "PositionParticle"
    "DrawAtlasSprite"
    "kMaximumAnimatedParticles = 64")
  string(FIND "${scene_contents}" "${required_scene_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Goodband scene must include ${required_scene_token}")
  endif()
endforeach()

string(FIND "${scene_contents}" "DrawLine" line_position)
if(NOT line_position EQUAL -1)
  message(FATAL_ERROR "Goodband magical animation must use particles, not line effects")
endif()
string(FIND "${scene_contents}" "FillCircle" circle_position)
if(NOT circle_position EQUAL -1)
  message(FATAL_ERROR "Goodband parameter animation must use textured sprites instead of circle particles")
endif()

file(READ "${backdrop_header}" backdrop_contents)
foreach(required_backdrop_token
    "kGestureCycleDurationMs = 20000"
    "kGestureWindowStart = 0.85F"
    "kGestureParticleCount = 56"
    "gestureBackground_"
    "vfxAtlas_"
    "DrawAtlasSprite"
    "DrawChiRibbon"
    "DrawGestureEnergy"
    "OnEndAnimation")
  string(FIND "${backdrop_contents}" "${required_backdrop_token}" backdrop_position)
  if(backdrop_position EQUAL -1)
    message(FATAL_ERROR "Goodband backdrop must include ${required_backdrop_token}")
  endif()
endforeach()

file(READ "${help_header}" help_contents)
foreach(required_help_token
    "THE THREEFOLD MANUAL"
    "\"THE THREEFOLD MANUAL\", 22.0F"
    "Three-band compression, harmonic saturation and tonal shaping"
    "CLEAN"
    "WARM"
    "PUNCH"
    "WIDE"
    "AMOUNT"
    "MIX"
    "OUTPUT"
    "FORMS / METERS / MATCH"
    "LOW, MID and HIGH show live gain reduction"
    "MATCH compensates perceived level"
    "HelpButtonBounds"
    "CloseButtonBounds"
    "bool IsHit")
  string(FIND "${help_contents}" "${required_help_token}" help_token_position)
  if(help_token_position EQUAL -1)
    message(FATAL_ERROR "Goodband help panel must include ${required_help_token}")
  endif()
endforeach()
string(FIND "${backdrop_contents}" "FillCircle" backdrop_circle_position)
if(NOT backdrop_circle_position EQUAL -1)
  message(FATAL_ERROR "Goodband Sensei gesture must use textured sprites instead of circle particles")
endif()

file(READ "${vfx_renderer_header}" vfx_renderer_contents)
foreach(required_vfx_token
    "kAtlasColumns = 4"
    "kAtlasRows = 4"
    "DrawAtlasSprite"
    "drawLayer(size * 1.24F"
    "clampedOpacity * 1.42F"
    "DrawChiRibbon"
    "PathCubicBezierTo"
    "PathStroke"
    "kOpacities[]{0.16F, 0.44F, 1.0F}")
  string(FIND "${vfx_renderer_contents}" "${required_vfx_token}" vfx_token_position)
  if(vfx_token_position EQUAL -1)
    message(FATAL_ERROR "Goodband chi VFX renderer must include ${required_vfx_token}")
  endif()
endforeach()

file(READ "${shuriken_header}" shuriken_contents)
foreach(required_shuriken_token
    "class IllustratedShurikenKnobControl"
    "DrawBitmapAspectFit(graphics, arena_"
    "DrawBitmapAspectFit(graphics, shuriken_"
    "DrawNeedle"
    "DrawSettingSparks")
  string(FIND "${shuriken_contents}" "${required_shuriken_token}" shuriken_position)
  if(shuriken_position EQUAL -1)
    message(FATAL_ERROR "Goodband shuriken control must include ${required_shuriken_token}")
  endif()
endforeach()

file(READ "${character_header}" character_contents)
foreach(required_character_token
    "class IllustratedCharacterControl"
    "DrawThreeSlicePlate"
    "unselectedPlate_"
    "selectedPlate_"
    "BeginEnergyStrike"
    "kEnergyStrikeDurationMs = 520")
  string(FIND "${character_contents}" "${required_character_token}" character_position)
  if(character_position EQUAL -1)
    message(FATAL_ERROR "Goodband fighting-game Character selector must include ${required_character_token}")
  endif()
endforeach()

file(READ "${plugin_source}" source_contents)
foreach(required_frame_token
    "kFrameOverscanScale = 1.048F"
    "bounds.GetScaledAboutCentre(kFrameOverscanScale)")
  string(FIND "${source_contents}" "${required_frame_token}" frame_token_position)
  if(frame_token_position EQUAL -1)
    message(FATAL_ERROR "Goodband frame must overscan to the plugin edges using ${required_frame_token}")
  endif()
endforeach()
string(FIND "${source_contents}" "new threefold::AspectFitBitmapControl" aspect_fit_bitmap_position)
if(aspect_fit_bitmap_position EQUAL -1)
  message(FATAL_ERROR "Goodband decorative bitmaps must preserve their natural aspect ratio")
endif()
string(FIND "${source_contents}" "LoadBitmap(TEMPLE_OF_MASTERY_BG_FN)" bitmap_position)
string(FIND "${source_contents}" "LoadBitmap(TEMPLE_OF_MASTERY_GESTURE_BG_FN)" gesture_bitmap_position)
string(FIND "${source_contents}" "new GoodbandBackdropControl" backdrop_position)
string(FIND "${source_contents}" "new GoodbandSceneControl" scene_position)
string(FIND "${source_contents}" "new IllustratedShurikenKnobControl" knob_position)
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
    "resources/img/character-selector-unselected.png"
    "resources/img/character-selector-unselected@2x.png"
    "resources/img/character-selector-selected.png"
    "resources/img/character-selector-selected@2x.png"
    "resources/img/throwing-star-knob.png"
    "resources/img/throwing-star-knob@2x.png"
    "resources/img/knob-arena.png"
    "resources/img/knob-arena@2x.png"
    "resources/img/threefold-palm-wordmark.png"
    "resources/img/threefold-palm-wordmark@2x.png"
    "resources/img/threefold-palm-frame.png"
    "resources/img/threefold-palm-frame@2x.png"
    "resources/img/chi-vfx-atlas.png"
    "resources/img/chi-vfx-atlas@2x.png")
  string(FIND "${cmake_contents}" "${resource_path}" asset_position)
  if(asset_position EQUAL -1)
    message(FATAL_ERROR "Goodband CMake resources must include ${resource_path}")
  endif()
endforeach()

string(FIND "${source_contents}" "GoodbandHelpControl" help_control_position)
if(help_control_position EQUAL -1)
  message(FATAL_ERROR "Goodband must attach its in-plugin help control")
endif()
string(FIND "${cmake_contents}" "GoodbandHelpControl.h" help_header_position)
if(help_header_position EQUAL -1)
  message(FATAL_ERROR "Goodband CMake sources must include its help control")
endif()

foreach(required_windows_cmake_token
    "if(WIN32)"
    "resources/main.rc"
    "Goodband-vst3"
    "Goodband-clap"
    "if(TARGET \${goodband_windows_target})")
  string(FIND "${cmake_contents}" "${required_windows_cmake_token}" windows_cmake_position)
  if(windows_cmake_position EQUAL -1)
    message(FATAL_ERROR "Goodband Windows build must include ${required_windows_cmake_token}")
  endif()
endforeach()

file(READ "${windows_resources}" windows_resource_contents)
foreach(required_windows_resource_token
    "TEMPLE_OF_MASTERY_BG_FN PNG"
    "THREEFOLD_PALM_WORDMARK_FN PNG"
    "THREEFOLD_PALM_FRAME_FN PNG"
    "THREEFOLD_PALM_CHI_VFX_ATLAS_FN PNG"
    "chi-vfx-atlas@2x.png")
  string(FIND "${windows_resource_contents}" "${required_windows_resource_token}" windows_resource_position)
  if(windows_resource_position EQUAL -1)
    message(FATAL_ERROR "Goodband Windows resources must include ${required_windows_resource_token}")
  endif()
endforeach()

foreach(required_preset_token
    "IllustratedCharacterControl"
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
