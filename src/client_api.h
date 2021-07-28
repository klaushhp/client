#ifndef _CLIENT_API_H_
#define _CLIENT_API_H_

#include "common.h"

client_err_t create_client(char* client_name);
void destory_client();
client_err_t connect_to_server(char* ip, uint16_t port);
client_err_t disconnect_from_server();
client_err_t get_login_status(bool* login_status);
client_err_t register_to_server();
client_err_t unregister_from_server();
client_err_t client_data_upload(void* payload, int len);

#endif