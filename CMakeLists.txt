cmake_minimum_required(VERSION 3.00)
project(libcpp)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

add_definitions(-Wall -std=c++17 -O3)

include_directories(include/)

add_subdirectory(src)

add_subdirectory(test)