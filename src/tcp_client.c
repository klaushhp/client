#include "tcp_client.h"

static tcp_client* client_tmp = NULL;

static void append_to_list(tcp_client* client)
{

}

static bool check_client_validity(tcp_client* client)
{
    return true;
}

static tcp_client* get_client_from_name(char* client_name)
{
    return client_tmp;
}

static client_err_t tcp_client_init(tcp_client* client, char* name)
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

client_err_t create_tcp_client(char* client_name)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    /*TODO: check instance validity*/
    /*Currently is use the singleton mode*/
    if(get_client_from_name(client_name) == NULL) {

        tcp_client* client = (tcp_client *)calloc(1, sizeof(tcp_client));
        if( client == NULL )
        {
            return CLIENT_ERR_NOMEN;
        }

        ret = tcp_client_init(client, client_name);
        if( ret == CLIENT_ERR_SUCCESS ) {
            append_to_list(client);
        } else {
            destory_tcp_client(client_name);
        }
    } else {
        ret = CLIENT_ERR_INVALID_PARAM;
        printf("err!: Instance %s is existed\n", client_name);
    }    

    return ret;
}

void destory_tcp_client(char* client_name)
{
    /*TODO: do disconnect*/
    /*TODO: cancel loop thread*/
    /*TODO: release resource*/

}

client_err_t connect_tcp_server(char* client_name, char* ip, uint16_t port)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    /*TODO: check instance validity*/
    tcp_client* client = get_client_from_name(client_name);

    if( client != NULL && check_client_validity(client) ) {
        tcp_client_state state = client_get_state(client); 
        if( state == tcp_cs_new || state == tcp_cs_disconnected ) {
            strncpy(client->address, ip, sizeof(client->address));
            client->port = port;

            send_internal_signal(client->sockpair_w, internal_cmd_connect);
        } else {
            ret = CLIENT_ERR_INVALID_OPERATION;
            printf("Invalid status, can't connect\n");
        }
    } else {
        ret = CLIENT_ERR_INVALID_PARAM;
    }

    return ret;
}

client_err_t disconnect_tcp_server(char* client_name)
{

}

client_err_t get_tcp_login_status(char* client_name, bool* login_status)
{

}

client_err_t register_to_tcp_server(char* client_name)
{

}

client_err_t unregister_from_tcp_server(char* client_name)
{

}

client_err_t tcp_client_data_upload(char* client_name, void* payload, int len)
{


}