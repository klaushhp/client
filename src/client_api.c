#include "client_api.h"
#include "tcp_client.h"


client_err_t create_client(char* name)
{
    return create_tcp_client(name);
}

void destory_client(char* client_name)
{
    destory_tcp_client(client_name);
}

client_err_t connect_to_server(char* client_name, char* ip, uint16_t port)
{
    return connect_tcp_server(client_name, ip, port);
}

client_err_t disconnect_from_server(char* client_name)
{
    return disconnect_tcp_server(client_name);
}

client_err_t get_login_status(char* client_name, bool* login_status)
{
    return get_tcp_login_status(client_name, login_status);
}

client_err_t register_to_server(char* client_name)
{
    return register_to_tcp_server(client_name);
}

client_err_t unregister_from_server(char* client_name)
{
    return unregister_from_tcp_server(client_name);
}

client_err_t client_data_upload(char* client_name, void* payload, int len)
{
    return tcp_client_data_upload(client_name, payload, len);
}


