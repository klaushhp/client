#include "client_api.h"

#ifdef USE_TCP
#include "tcp_client.h"
#elif defined(USE_MQTT)

#endif

net_client* create_client(const char* name)
{    
    return create_tcp_client(name);
}

void destory_client(net_client* client)
{
    destory_tcp_client(client);
}

client_err_t connect_to_server(net_client* client, const char* ip, uint16_t port)
{
    return connect_tcp_server(client, ip, port);
}

client_err_t disconnect_from_server(net_client* client)
{
    return disconnect_tcp_server(client);
}

client_err_t get_login_status(net_client* client, bool* login_status)
{
    return get_tcp_login_status(client, login_status);
}

client_err_t register_to_server(net_client* client)
{
    return register_to_tcp_server(client);
}

client_err_t unregister_from_server(net_client* client)
{
    return unregister_from_tcp_server(client);
}

client_err_t client_data_upload(net_client* client, const void* payload, int len)
{
    return tcp_client_data_upload(client, payload, len);
}


