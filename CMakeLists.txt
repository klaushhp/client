project(client)

cmake_minimum_required(VERSION 3.20)

aux_source_directory(./src SOURCE)
aux_source_directory(./src/tcp_client SOURCE)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/tcp_client)

#set(SRC_LIST main.c client_api.c tcp_client.c tcp_socket.c tcp_packet.c common.c)

add_definitions(-DUSE_TCP)

add_executable(client ${SOURCE})

target_link_libraries(client pthread)
