iplug_add_plugin(Goodband
  SOURCES
    Goodband.cpp
    Goodband.h
    GoodbandBackdropControl.h
    GoodbandCharacterPresets.h
    GoodbandHelpControl.h
    ChiVfxRenderer.h
    GoodbandSceneControl.h
    FightingGameCharacterControl.h
    IllustratedBitmapDrawing.h
    IllustratedShurikenKnobControl.h
    ../shared/WaveFactoryUI.h
    resources/resource.h
  RESOURCES
    resources/img/temple-of-mastery-bg.png
    resources/img/temple-of-mastery-bg@2x.png
    resources/img/temple-of-mastery-gesture-bg.png
    resources/img/temple-of-mastery-gesture-bg@2x.png
    resources/img/character-selector-unselected.png
    resources/img/character-selector-unselected@2x.png
    resources/img/character-selector-selected.png
    resources/img/character-selector-selected@2x.png
    resources/img/throwing-star-knob.png
    resources/img/throwing-star-knob@2x.png
    resources/img/knob-arena.png
    resources/img/knob-arena@2x.png
    resources/img/threefold-palm-wordmark.png
    resources/img/threefold-palm-wordmark@2x.png
    resources/img/threefold-palm-frame.png
    resources/img/threefold-palm-frame@2x.png
    resources/img/chi-vfx-atlas.png
    resources/img/chi-vfx-atlas@2x.png
  FORMATS
    VST3 AU CLAP
  LINK
    wave_factory_dsp
)

if(WIN32)
  set(goodband_windows_resources resources/main.rc)
  set_source_files_properties(${goodband_windows_resources} PROPERTIES
    COMPILE_FLAGS "/I\"${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts\" /I\"${CMAKE_CURRENT_SOURCE_DIR}/resources/img\" /I\"${CMAKE_CURRENT_SOURCE_DIR}/resources\""
  )
  foreach(goodband_windows_target Goodband-vst3 Goodband-clap)
    if(TARGET ${goodband_windows_target})
      target_sources(${goodband_windows_target} PRIVATE ${goodband_windows_resources})
    endif()
  endforeach()
endif()
