set(padsampler_includes
  "${IPLUG2_DIR}/IPlug" "${IPLUG2_DIR}/IPlug/Extras"
  "${IPLUG2_DIR}/WDL" "${IPLUG2_DIR}/Dependencies/Extras/nlohmann")
if(BUILD_TESTING)
  add_executable(padsampler-tests sampler_tests.cpp AppleSampleLoader.cpp)
  target_include_directories(padsampler-tests PRIVATE ${padsampler_includes})
  target_link_libraries(padsampler-tests PRIVATE "-framework AudioToolbox" "-framework CoreFoundation")
  add_test(NAME padsampler-tests COMMAND padsampler-tests)
  if(TARGET wfe-starter-checks)
    add_dependencies(wfe-starter-checks padsampler-tests)
  endif()
endif()
if(WFE_BUILD_PLUGINS)
  iplug_add_plugin(PadSampler
    SOURCES PadSampler.cpp PadSampler.h SamplerEngine.h PadFX.h FxDSP.h IPlugFilter.h SampleLibrary.h SampleLoader.h AppleSampleLoader.cpp resources/resource.h
    RESOURCES resources/img/precision-satin.png resources/img/precision-satin@2x.png
    FORMATS APP AU VST3)
  foreach(format app au vst3)
    set_target_properties(PadSampler-${format} PROPERTIES XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "12.0")
    target_include_directories(PadSampler-${format} PRIVATE ${padsampler_includes})
    target_link_libraries(PadSampler-${format} PRIVATE "-framework AudioToolbox" "-framework CoreFoundation")
  endforeach()
  add_custom_target(padsampler-formats DEPENDS PadSampler-app PadSampler-au PadSampler-vst3)
endif()
