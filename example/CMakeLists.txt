set(CMAKE_CXX_STANDARD 20)

add_executable(echo_server "echo_server.cc")
target_link_libraries(echo_server net rpc_server pthread)

add_executable(echo_client "echo_client.cc")
target_link_libraries(echo_client net rpc_client pthread)
