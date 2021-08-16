#ifndef _CLIENT_API_H_
#define _CLIENT_API_H_

#include "common.h"

client_t connect_to_server(const connect_opt* opt);
client_err_t disconnect_from_server(client_t handle);
client_err_t client_data_upload(client_t handle, const void* payload, int len);

//client_err_t login_to_server(client_t handle);
//client_err_t logout_from_server(client_t handle);
//client_err_t get_login_status(client_t handle, bool* login_status);



#endif