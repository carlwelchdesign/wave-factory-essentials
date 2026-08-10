add_executable(wave_factory_dsp_tests dsp_tests.cpp)
target_link_libraries(wave_factory_dsp_tests PRIVATE wave_factory_dsp)

add_executable(goodband_character_preset_tests goodband_character_preset_tests.cpp)
target_include_directories(goodband_character_preset_tests PRIVATE ${PROJECT_SOURCE_DIR})

if(MSVC)
  target_compile_options(wave_factory_dsp_tests PRIVATE /W4 /permissive-)
  target_compile_options(goodband_character_preset_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(wave_factory_dsp_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  target_compile_options(goodband_character_preset_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_test(NAME wave_factory_dsp_tests COMMAND wave_factory_dsp_tests)
add_test(NAME goodband_character_preset_tests COMMAND goodband_character_preset_tests)

add_test(
  NAME bundle_metadata_test
  COMMAND ${CMAKE_COMMAND}
    -DPROJECT_ROOT=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_bundle_metadata.cmake
)

add_test(
  NAME ui_font_contract_test
  COMMAND ${CMAKE_COMMAND}
    -DPROJECT_ROOT=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_ui_font_contract.cmake
)

add_test(
  NAME goodband_scene_contract_test
  COMMAND ${CMAKE_COMMAND}
    -DPROJECT_ROOT=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_goodband_scene_contract.cmake
)
