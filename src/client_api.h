#ifndef _CLIENT_API_H_
#define _CLIENT_API_H_

#include "common.h"

net_client* create_client(const char* client_name);
void destory_client(net_client* client);
client_err_t connect_to_server(net_client* client, const char* ip, uint16_t port);
client_err_t disconnect_from_server(net_client* client);
client_err_t get_login_status(net_client* client, bool* login_status);
client_err_t register_to_server(net_client* client);
client_err_t unregister_from_server(net_client* client);
client_err_t client_data_upload(net_client* client, const void* payload, int len);

//client_err_t connect_to_server(const char* client_name, const char* ip, uint16_t port);
//client_err_t disconnect_from_server(const char* client_name);
//client_err_t get_login_status(const char* client_name, bool* login_status);
//client_err_t register_to_server(const char* client_name);
//client_err_t unregister_from_server(const char* client_name);
//client_err_t client_data_upload(const char* client_name, const void* payload, int len);

#endif