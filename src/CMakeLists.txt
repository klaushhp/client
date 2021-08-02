project(client)

cmake_minimum_required(VERSION 3.20)

set(SRC_LIST main.c client_api.c tcp_client.c tcp_socket.c tcp_packet.c)

add_definitions(-DUSE_TCP)

add_executable(client ${SRC_LIST})

target_link_libraries(client pthread)
