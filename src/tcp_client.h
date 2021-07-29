#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include "common.h"
#include "tcp_socket.h"

client_err_t create_tcp_client(char* name);
void destory_tcp_client();
client_err_t connect_to_tcp_server(char* ip, uint16_t port);


#endif