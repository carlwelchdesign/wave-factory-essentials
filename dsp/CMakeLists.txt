add_library(wave_factory_dsp STATIC
  src/goodband_processor.cpp
  src/pitch_trails_processor.cpp
)

target_include_directories(wave_factory_dsp PUBLIC include)
target_compile_features(wave_factory_dsp PUBLIC cxx_std_17)

if(MSVC)
  target_compile_options(wave_factory_dsp PRIVATE /W4 /permissive-)
else()
  target_compile_options(wave_factory_dsp PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()
