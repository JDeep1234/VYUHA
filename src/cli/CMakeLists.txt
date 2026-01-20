# CLI Module - CMake
# Author: Jdeep

add_library(cli_module STATIC
    cli.cpp
)

target_include_directories(cli_module PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
