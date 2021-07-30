#ifndef _CLIENT_API_H_
#define _CLIENT_API_H_

#include "common.h"

client_err_t create_client(char* client_name);
void destory_client(char* client_name);
client_err_t connect_to_server(char* client_name, char* ip, uint16_t port);
client_err_t disconnect_from_server(char* client_name);
client_err_t get_login_status(char* client_name, bool* login_status);
client_err_t register_to_server(char* client_name);
client_err_t unregister_from_server(char* client_name);
client_err_t client_data_upload(char* client_name, void* payload, int len);

#endif