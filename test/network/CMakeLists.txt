add_executable(clinet clinet.cpp)
add_executable(service service.cpp)

target_link_libraries(clinet pthread rt boost_system boost_coroutine)
target_link_libraries(service pthread rt boost_system boost_coroutine)