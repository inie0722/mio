project(test)

add_subdirectory(parallelism)

add_executable(tsdb tsdb.cpp)

target_link_libraries(tsdb gtest pthread)

add_executable(log log.cpp)

target_link_libraries(log gtest pthread fmt)