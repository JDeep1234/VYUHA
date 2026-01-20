# Integration Module - CMake
# Author: Jdeep

add_library(integration_module STATIC
    integration_manager.cpp
    edr_connector.cpp
    snapshot_manager.cpp
    clean_module.cpp
)

target_include_directories(integration_module PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

if(WIN32)
    target_link_libraries(integration_module PUBLIC
        advapi32
        wbemuuid
        ole32
        oleaut32
    )
endif()
