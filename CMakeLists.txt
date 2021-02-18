cmake_minimum_required(VERSION 3.00)

project(mio)

set(CMAKE_CXX_STANDARD 17)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/fmt/include)
include_directories(${CMAKE_SOURCE_DIR}/parallel-hashmap)

add_definitions(-Wall)

add_subdirectory(src)

add_subdirectory(test)

add_subdirectory(fmt)