cmake_minimum_required(VERSION 3.25)

if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum supported macOS version" FORCE)
endif()

project(WaveFactoryEssentials VERSION 0.1.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WFE_BUILD_PLUGINS "Build the iPlug2 plugin targets" ON)

add_subdirectory(dsp)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

if(WFE_BUILD_PLUGINS)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/iPlug2" CACHE PATH "Path to iPlug2")
  if(NOT EXISTS "${IPLUG2_DIR}/iPlug2.cmake")
    message(FATAL_ERROR "iPlug2 is missing. Run: git submodule update --init --recursive")
  endif()

  include("${IPLUG2_DIR}/iPlug2.cmake")
  find_package(iPlug2 REQUIRED)

  add_subdirectory(plugins/Goodband)
  add_subdirectory(plugins/PitchTrails)
endif()
