#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include "common.h"
#include "tcp_socket.h"

client_err_t create_tcp_client(char* client_name);
void destory_tcp_client(char* client_name);
client_err_t connect_tcp_server(char* client_name, char* ip, uint16_t port);
client_err_t disconnect_tcp_server(char* client_name);
client_err_t get_tcp_login_status(char* client_name, bool* login_status);
client_err_t register_to_tcp_server(char* client_name);
client_err_t unregister_from_tcp_server(char* client_name);
client_err_t tcp_client_data_upload(char* client_name, void* payload, int len);

#endif