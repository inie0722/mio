project(test)

add_subdirectory(parallelism)

add_executable(log log.cpp)

target_link_libraries(log pthread)