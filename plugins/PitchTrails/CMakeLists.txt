iplug_add_plugin(PitchTrails
  SOURCES
    PitchTrails.cpp
    PitchTrails.h
    ../shared/WaveFactoryUI.h
    resources/resource.h
  FORMATS
    VST3 AU CLAP
  LINK
    wave_factory_dsp
)
