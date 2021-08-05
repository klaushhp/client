#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include "common.h"
#include "tcp_socket.h"

tcp_client* create_tcp_client();
void destroy_tcp_client(tcp_client* client);
client_err_t connect_tcp_server(tcp_client* client, const char* host, uint16_t port);
client_err_t disconnect_tcp_server(tcp_client* client);
client_err_t start_tcp_main_loop(tcp_client* client);
client_err_t stop_tcp_main_loop(tcp_client* client);
client_err_t tcp_client_data_upload(tcp_client* client, const void* payload, int len);
client_err_t get_tcp_login_status(tcp_client* client, bool* login_status);
client_err_t register_to_tcp_server(tcp_client* client);
client_err_t unregister_from_tcp_server(tcp_client* client);

#endif