cmake_minimum_required(VERSION 3.00)
project(libcpp)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-Wall)

include_directories(include/)

add_subdirectory(src)

add_subdirectory(test)