# Agent Core Module - CMake
# Author: Jdeep

add_library(agent_core STATIC
    agent.cpp
    orchestrator.cpp
    output_handler.cpp
    cleaner.cpp
    telemetry.cpp
)

target_include_directories(agent_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

if(WIN32)
    target_link_libraries(agent_core PUBLIC
        advapi32
        wevtapi
    )
endif()
