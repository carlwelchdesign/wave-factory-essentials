iplug_add_plugin(Goodband
  SOURCES
    Goodband.cpp
    Goodband.h
    ../shared/WaveFactoryUI.h
    resources/resource.h
  FORMATS
    VST3 AU CLAP
  LINK
    wave_factory_dsp
)
