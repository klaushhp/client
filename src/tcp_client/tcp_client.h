#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include "common.h"
#include "tcp_socket.h"

net_client* create_tcp_client(const char* client_name);
void destroy_tcp_client(net_client* client);
client_err_t connect_tcp_server(net_client* client, const char* ip, uint16_t port);
client_err_t disconnect_tcp_server(net_client* client);
client_err_t get_tcp_login_status(net_client* client, bool* login_status);
client_err_t register_to_tcp_server(net_client* client);
client_err_t unregister_from_tcp_server(net_client* client);
client_err_t tcp_client_data_upload(net_client* client, const void* payload, int len);

#endif