set(CMAKE_CXX_STANDARD 20)

add_executable(echo_server "echo_server.cc")
target_link_libraries(echo_server net rpc_server pthread)

# add_executable(client "client.cc")
# target_link_libraries(client core base pthread)
