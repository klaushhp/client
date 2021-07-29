project(client)

set(SRC_LIST main.c client_api.c tcp_client.c tcp_socket.c)

add_executable(client ${SRC_LIST})

target_link_libraries(client pthread)
