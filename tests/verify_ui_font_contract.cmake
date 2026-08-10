function(verify_plugin_ui_font plugin_name)
  set(source_path "${PROJECT_ROOT}/plugins/${plugin_name}/${plugin_name}.cpp")
  file(READ "${source_path}" source_contents)

  string(FIND "${source_contents}" "graphics->LoadFont(DEFAULT_FONT, \"Arial\", ETextStyle::Normal);" font_position)
  string(FIND "${source_contents}" "graphics->AttachControl" control_position)
  if(plugin_name STREQUAL "Goodband")
    set(parameter_control_type "new IllustratedShurikenKnobControl")
  else()
    set(parameter_control_type "new IVKnobControl")
  endif()
  string(FIND "${source_contents}" "${parameter_control_type}" parameter_control_position)

  if(font_position EQUAL -1)
    message(FATAL_ERROR "${plugin_name} must load the default UI font")
  endif()

  if(control_position EQUAL -1 OR NOT font_position LESS control_position)
    message(FATAL_ERROR "${plugin_name} must load the UI font before attaching text controls")
  endif()

  string(FIND "${source_contents}" ", \"Arial\", EAlign::" unregistered_font_id_position)
  if(NOT unregistered_font_id_position EQUAL -1)
    message(FATAL_ERROR
      "${plugin_name} text styles must reference DEFAULT_FONT, not the Arial system-font name")
  endif()

  if(plugin_name STREQUAL "Goodband")
    set(style_factory "MakeCinematicControlStyle")
  else()
    set(style_factory "MakeDarkControlStyle")
  endif()
  string(FIND "${source_contents}" "${style_factory}" style_position)
  if(style_position EQUAL -1 OR parameter_control_position EQUAL -1 OR NOT style_position LESS parameter_control_position)
    message(FATAL_ERROR "${plugin_name} must create its high-contrast control style before attaching parameter controls")
  endif()
endfunction()

verify_plugin_ui_font("Goodband")
verify_plugin_ui_font("PitchTrails")

file(READ "${PROJECT_ROOT}/plugins/Goodband/Goodband.cpp" goodband_source)
string(FIND "${goodband_source}" "LoadBitmap(THREEFOLD_PALM_WORDMARK_FN)" wordmark_load_position)
string(FIND "${goodband_source}" "new FittedBitmapControl(IRECT(27.0F, 12.0F, 410.0F, 91.0F), wordmark)" wordmark_control_position)
if(wordmark_load_position EQUAL -1 OR wordmark_control_position EQUAL -1 OR
   NOT wordmark_load_position LESS wordmark_control_position)
  message(FATAL_ERROR "Goodband must load its illustrated wordmark before attaching it")
endif()

file(READ "${PROJECT_ROOT}/plugins/shared/WaveFactoryUI.h" style_contents)
foreach(required_style_token
    "WithShowLabel(true)"
    "WithShowValue(true)"
    "WithLabelText"
    "WithValueText")
  string(FIND "${style_contents}" "${required_style_token}" token_position)
  if(token_position EQUAL -1)
    message(FATAL_ERROR "Shared control style must include ${required_style_token}")
  endif()
endforeach()
