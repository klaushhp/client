#include "tcp_client.h"

static tcp_client* client = NULL;


static client_err_t tcp_client_init(char* name)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    strncpy(client->client_name, name, sizeof(client->client_name));
    client->sockfd = INVALID_SOCKET;
    memset(client->address, 0, sizeof(client->address));
    client->port = 0;
    client->state = tcp_cs_new;
    client->login_status = false;
    client->msg = NULL;

    pthread_mutex_init(&client->state_mutex, NULL);
    pthread_mutex_init(&client->packet_mutex, NULL);

    if( local_socketpair(&client->sockpair_r, &client->sockpair_w) )
    {
        ret = CLIENT_ERR_ERRNO;
    }

    if( ret ==  CLIENT_ERR_SUCCESS )
    {
        if( pthread_create(&client->thread_id, NULL, tcp_client_main_loop, (void *)client) == 0 )
        {
            pthread_detach(client->thread_id);
        }
        else
        {
            ret = CLIENT_ERR_ERRNO;
        }     
    }

    return ret;
}

client_err_t create_tcp_client(char* name)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    if( client != NULL )
    {
        return CLIENT_ERR_SINGLETON;
    }

    client = (tcp_client *)calloc(1, sizeof(tcp_client));
    if( client == NULL )
    {
        return CLIENT_ERR_NOMEN;
    }

    ret = tcp_client_init(name) ;
    if( ret )
    {
        destory_tcp_client();
    }

    return ret;
}

void destory_tcp_client()
{



}

client_err_t connect_to_tcp_server(char* ip, uint16_t port)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    if( (client != NULL) && (client_get_state(client) != tcp_cs_connected) )
    {
        strncpy(client->address, ip, sizeof(client->address));
        client->port = port;

        send_internal_signal(client->sockpair_w, internal_cmd_connect);
    }
    else
    {
        ret = CLIENT_ERR_INVALID_OPERATION;
    }

    return ret;
}