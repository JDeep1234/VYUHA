# ============================================================================
# ML FRAMEWORK MODULE - CMake
# ============================================================================
# Author: KARTHIK
# 
# KARTHIK: Add your ML source files here!
# ============================================================================

add_library(ml_framework STATIC
    ml_engine.cpp
    
    # ==========================================================================
    # KARTHIK: ADD YOUR ML SOURCE FILES BELOW
    # ==========================================================================
    # Example:
    # detection_analyzer.cpp
    # evasion_scorer.cpp
    # event_correlator.cpp
    # behavior_classifier.cpp
    # anomaly_detector.cpp
    # 
    # Each file should implement one or more ML components
    # ==========================================================================
)

target_include_directories(ml_framework PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# KARTHIK: Uncomment when you have ML library dependencies
# ============================================================================
# find_package(OpenCV QUIET)
# if(OpenCV_FOUND)
#     target_link_libraries(ml_framework PUBLIC ${OpenCV_LIBS})
# endif()
# 
# find_package(TensorFlow QUIET)
# if(TensorFlow_FOUND)
#     target_link_libraries(ml_framework PUBLIC tensorflow)
# endif()
