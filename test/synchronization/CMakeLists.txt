add_executable(spin_mutex spin_mutex.cpp)

target_link_libraries(spin_mutex pthread)

add_executable(spin_shared_mutex spin_shared_mutex.cpp)

target_link_libraries(spin_shared_mutex pthread tbb)