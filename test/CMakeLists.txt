project(test)



add_subdirectory(parallelism)

add_subdirectory(synchronization)

add_subdirectory(network)

add_executable(log log.cpp)

target_link_libraries(log rt boost_system pthread fmt)

add_executable(c c.cpp)

target_link_libraries(c rt boost_system pthread boost_coroutine boost_context boost_fiber)

add_executable(s s.cpp)

target_link_libraries(s rt boost_system pthread boost_coroutine boost_context boost_fiber)

add_executable(test test.cpp)

target_link_libraries(test rt boost_system pthread fmt)