# veronicarecomp - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(veronicarecomp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(VERONICARECOMP_SOURCES
    src/main.cpp
    src/veronicarecomp_settings.cpp
    src/veronicarecomp_ring_backoff.cpp
)

if(WIN32)
    add_executable(veronicarecomp WIN32 ${VERONICARECOMP_SOURCES})
else()
    add_executable(veronicarecomp ${VERONICARECOMP_SOURCES})
endif()

rexglue_setup_target(veronicarecomp)
