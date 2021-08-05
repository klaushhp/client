#ifndef _TCP_SOCKET_H_
#define _TCP_SOCKET_H_

#include "common.h"
#include "tcp_packet.h"

client_err_t set_socket_nonblock(int* sock);
client_err_t local_socketpair(int* pair_r, int* pair_w);
client_err_t connect_server(tcp_client* client);
void net_socket_close(tcp_client* client);
void* tcp_client_main_loop(void* obj);



#endif