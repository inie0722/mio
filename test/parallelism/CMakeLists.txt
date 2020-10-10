add_executable(disruptor disruptor.cpp)

add_executable(ring_queue ring_queue.cpp)

add_executable(pipe pipe.cpp)

add_executable(boost_queue boost_queue.cpp)

add_executable(boost_spsc_queue boost_spsc_queue.cpp)

#add_executable(test test.cpp)

target_link_libraries(disruptor pthread)

target_link_libraries(ring_queue pthread)

target_link_libraries(pipe pthread boost_system)

target_link_libraries(boost_queue pthread)

target_link_libraries(boost_spsc_queue pthread)

#target_link_libraries(test pthread)