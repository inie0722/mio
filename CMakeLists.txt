cmake_minimum_required(VERSION 3.00)
project(InNET)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

add_definitions(-Wall -O3 -std=gnu++17)

include_directories(include/)

add_subdirectory(src)

add_subdirectory(test)