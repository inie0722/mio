add_executable(aba_ptr aba_ptr.cpp)

target_link_libraries(aba_ptr gtest pthread)

add_executable(allocator allocator.cpp)

target_link_libraries(allocator gtest pthread)