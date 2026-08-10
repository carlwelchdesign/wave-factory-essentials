add_executable(wave_factory_dsp_tests dsp_tests.cpp)
target_link_libraries(wave_factory_dsp_tests PRIVATE wave_factory_dsp)

if(MSVC)
  target_compile_options(wave_factory_dsp_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(wave_factory_dsp_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_test(NAME wave_factory_dsp_tests COMMAND wave_factory_dsp_tests)
