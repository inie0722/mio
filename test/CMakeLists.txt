project(test)

add_subdirectory(parallelism)

add_executable(test test.cpp)

target_link_libraries(test fiber)