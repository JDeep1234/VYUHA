# VYUHA: Cross-Layer EDR Kill-Chain Evasion via Deep Reinforcement Learning-Guided Adversarial Orchestration - C++ Build Configuration
# =================================================
# Build: mkdir build && cd build && cmake .. && cmake --build .

cmake_minimum_required(VERSION 3.16)
project(EDRAdaptiveFramework VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00)  # Windows 10
    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-DNOMINMAX)  # Prevent min/max macro conflicts
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ==============================================================================
# INCLUDE DIRECTORIES
# ==============================================================================
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# ==============================================================================
# JDEEP's MODULES - CLI, Agent Core, Integration
# ==============================================================================
add_subdirectory(src/cli)
add_subdirectory(src/agent_core)
add_subdirectory(src/integration)

# ==============================================================================
# BIPIN's MODULES - Exploit Scripts (C++ based attacks)
# ==============================================================================
add_subdirectory(src/exploits)

# ==============================================================================
# KARTHIK's MODULES - ML Framework
# ==============================================================================
add_subdirectory(src/ml_framework)

# ==============================================================================
# MAIN EXECUTABLE
# ==============================================================================
add_executable(edr_framework
    src/main.cpp
)

target_link_libraries(edr_framework PRIVATE
    cli_module
    agent_core
    integration_module
    exploit_scripts      # Bipin's module
    ml_framework         # Karthik's module
)

# Windows libraries
if(WIN32)
    target_link_libraries(edr_framework PRIVATE
        advapi32
        user32
        shell32
        ole32
        oleaut32
        wbemuuid
    )
endif()

# ==============================================================================
# INSTALL TARGETS
# ==============================================================================
install(TARGETS edr_framework
    RUNTIME DESTINATION bin
)

install(DIRECTORY config/
    DESTINATION config
)

# ==============================================================================
# TESTING (Optional)
# ==============================================================================
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "")
message(STATUS "=== VYUHA v${PROJECT_VERSION} ===")
message(STATUS "Team Members:")
message(STATUS "  - Jdeep:   CLI, Agent Core, Integration")
message(STATUS "  - Bipin:   Exploit Scripts Module")
message(STATUS "  - Karthik: ML Framework")
message(STATUS "")
