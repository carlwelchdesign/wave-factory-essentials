function(verify_plugin_ui_font plugin_name)
  set(source_path "${PROJECT_ROOT}/plugins/${plugin_name}/${plugin_name}.cpp")
  file(READ "${source_path}" source_contents)

  string(FIND "${source_contents}" "graphics->LoadFont(DEFAULT_FONT, \"Arial\", ETextStyle::Normal);" font_position)
  string(FIND "${source_contents}" "graphics->AttachControl" control_position)

  if(font_position EQUAL -1)
    message(FATAL_ERROR "${plugin_name} must load the default UI font")
  endif()

  if(control_position EQUAL -1 OR NOT font_position LESS control_position)
    message(FATAL_ERROR "${plugin_name} must load the UI font before attaching text controls")
  endif()
endfunction()

verify_plugin_ui_font("Goodband")
verify_plugin_ui_font("PitchTrails")
