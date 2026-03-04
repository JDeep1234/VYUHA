# ============================================================================
# ML FRAMEWORK MODULE - CMake
# ============================================================================
# Author: KARTHIK
# 
# KARTHIK: Add your ML source files here!
# ============================================================================

add_library(ml_framework STATIC
    ml_engine.cpp
    detection_analyzer.cpp
    evasion_scorer.cpp
    event_correlator.cpp
    ml_bridge.cpp
)

target_include_directories(ml_framework PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# ml_bridge.cpp uses CreateProcess / Windows pipe APIs
if(WIN32)
    target_link_libraries(ml_framework PUBLIC kernel32)
endif()
