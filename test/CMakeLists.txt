project(test)

add_subdirectory(parallelism)

add_subdirectory(synchronization)

add_executable(log log.cpp)

target_link_libraries(log pthread)

add_executable(test test.cpp)

target_link_libraries(test fiber)