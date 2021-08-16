#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include "common.h"
#include "tcp_socket.h"
#include "message_handler.h"

tcp_client* create_tcp_client();
void destroy_tcp_client(remote_client_t* client);
client_err_t connect_tcp_server(remote_client_t* client, const connect_opt* opt);
client_err_t disconnect_tcp_server(remote_client_t* client);
client_err_t start_tcp_main_loop(remote_client_t* client);
void stop_tcp_main_loop(remote_client_t* client);
client_err_t tcp_client_data_upload(remote_client_t* client, const void* payload, int len);
//client_err_t get_tcp_login_status(tcp_client* client, bool* login_status);
//client_err_t register_to_tcp_server(tcp_client* client);
//client_err_t unregister_from_tcp_server(tcp_client* client);

#endif