iplug_add_plugin(Goodband
  SOURCES
    Goodband.cpp
    Goodband.h
    GoodbandSceneControl.h
    ../shared/WaveFactoryUI.h
    resources/resource.h
  RESOURCES
    resources/img/arcade-dojo-bg.png
    resources/img/arcade-dojo-bg@2x.png
  FORMATS
    VST3 AU CLAP
  LINK
    wave_factory_dsp
)
