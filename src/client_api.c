#include "client_api.h"
#include "tcp_client.h"


client_err_t create_client(char* name)
{
    return create_tcp_client(name);
}


void destory_client()
{
    return destory_tcp_client();
}

client_err_t connect_to_server(char* ip, uint16_t port)
{
    return connect_to_tcp_server(ip, port);
}

client_err_t disconnect_from_server()
{

}

client_err_t get_login_status(bool* login_status)
{

}

client_err_t register_to_server()
{

}

client_err_t unregister_from_server()
{

}

client_err_t client_data_upload(void* payload, int len)
{


}


