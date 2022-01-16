add_executable(aba_ptr aba_ptr.cpp)

target_link_libraries(aba_ptr gtest pthread)

add_executable(allocator allocator.cpp)

target_link_libraries(allocator gtest pthread)

add_executable(spsc_queue spsc_queue.cpp)

target_link_libraries(spsc_queue gtest pthread)

add_executable(stack stack.cpp)

target_link_libraries(stack gtest pthread)

add_executable(queue queue.cpp)

target_link_libraries(queue gtest pthread)

add_executable(ring_queue ring_queue.cpp)

target_link_libraries(ring_queue gtest pthread)