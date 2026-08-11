iplug_add_plugin(PitchTrails
  SOURCES
    PitchTrails.cpp
    PitchTrails.h
    IllustratedSpiritDialControl.h
    ValleySpiritBitmapControl.h
    ValleySpiritHelpControl.h
    ValleySpiritSceneControl.h
    ../shared/WaveFactoryUI.h
    resources/resource.h
  RESOURCES
    resources/fonts/Marcellus-Regular.ttf
    resources/img/valley-spirit-bg.png
    resources/img/valley-spirit-bg@2x.png
    resources/img/valley-spirit-frame.png
    resources/img/valley-spirit-frame@2x.png
    resources/img/moonstone-dial.png
    resources/img/moonstone-dial@2x.png
    resources/img/jade-dial.png
    resources/img/jade-dial@2x.png
  FORMATS
    VST3 AU CLAP
  LINK
    wave_factory_dsp
)

if(WIN32)
  set(pitch_trails_windows_resources resources/main.rc)
  set_source_files_properties(${pitch_trails_windows_resources} PROPERTIES
    COMPILE_FLAGS "/I\"${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts\" /I\"${CMAKE_CURRENT_SOURCE_DIR}/resources/img\" /I\"${CMAKE_CURRENT_SOURCE_DIR}/resources\""
  )
  foreach(pitch_trails_windows_target PitchTrails-vst3 PitchTrails-clap)
    if(TARGET ${pitch_trails_windows_target})
      target_sources(${pitch_trails_windows_target} PRIVATE ${pitch_trails_windows_resources})
    endif()
  endforeach()
endif()
