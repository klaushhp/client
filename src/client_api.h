#ifndef _CLIENT_API_H_
#define _CLIENT_API_H_

#include "common.h"

client_err_t create_client(const char* client_name);
void destory_client(const char* client_name);
client_err_t connect_to_server(const char* client_name, const char* ip, uint16_t port);
client_err_t disconnect_from_server(const char* client_name);
client_err_t get_login_status(const char* client_name, bool* login_status);
client_err_t register_to_server(const char* client_name);
client_err_t unregister_from_server(const char* client_name);
client_err_t client_data_upload(const char* client_name,const void* payload, int len);

#endif