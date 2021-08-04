#include "client_api.h"
//#include "tcp_client.h"

static remote_client_t* client_list = NULL;
static uint32_t fd;

#if 0
static remote_client_t* get_client_by_name(const char* client_name)
{
    remote_client_t* client = NULL;

    DL_FOREACH(client_list, client) {
        if( strncmp(client->client_name, client_name, strlen(client->client_name)) == 0 )
        {
           break; 
        }
    }

    return client;
}
#endif

static remote_client_t* get_client_by_ip_port(const char* ip, uint16_t port)
{
    remote_client_t* client = NULL;

    DL_FOREACH(client_list, client) {
        if( strncmp(client->address, ip, strlen(client->address)) == 0 && 
        client->port == port ) {
            break;
        }
    }

    return client;
}

static remote_client_t* get_client_handle(client_t handle)
{
    remote_client_t* client = NULL;

    DL_FOREACH(client_list, client) {
        if( client->handle == handle ) {
            break;
        }
    }

    return client;
}

static client_err_t launch_main_loop(remote_client_t* client)
{


}

static client_err_t stop_main_loop(remote_client_t* client)
{


}

static client_t handle_alloc()
{
    return fd++;
}

static void handle_free()
{


}

static client_err_t connect_to_remote_server(remote_client_t* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    switch (client->type)
    {
    case PROTOCAL_TCP:
        //ret = connect_tcp_server(client);
        break;

    case PROTOCAL_MQTT:
        printf("Error: MQTT protocal is not supproted\n");
        ret = CLIENT_ERR_INVALID_PARAM;
        break;

    default:
        printf("Error: unkonwn protocal\n");
        ret = CLIENT_ERR_INVALID_PARAM;
        break;
    }

    return ret;
}

static client_err_t disconnect_remote_server(remote_client_t* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    switch (client->type)
    {
    case PROTOCAL_TCP:
        //ret = disconnect_tcp_server(client);
        break;

    case PROTOCAL_MQTT:
        printf("Error: MQTT protocal is not supproted\n");
        ret = CLIENT_ERR_INVALID_PARAM;
        break;

    default:
        printf("Error: unkonwn protocal\n");
        ret = CLIENT_ERR_INVALID_PARAM;
        break;
    }

    return ret;
}


static remote_client_t* create_remote_client(client_protocal_type type, const char* ip, uint16_t port)
{
    remote_client_t* client = NULL;

    client = (remote_client_t *)calloc(1, sizeof(remote_client_t));
    if( client != NULL ) {
        client->handle = handle_alloc();
        if( client->handle != INVALID_HANDLE ) {
            client->type = type;
            client->port = port;
            strncpy(client->address, ip, strlen(client->address));
            //client->login_status = false;
            //pthread_mutex_init(&client->login_mutex, NULL);

            switch (type) {
            case PROTOCAL_TCP:
                //client->clt = create_tcp_client();
                if( client->clt == NULL ) {
                    handle_free(client->handle);
                    free(client);    
                }
                break;
            
            case PROTOCAL_MQTT:
                printf("Error: MQTT protocal is not supproted\n");
                break;

            default:
                printf("Error: unkonwn protocal\n");
                break;
            }
        } else {
            free(client);
            client = NULL;
        }
    }

    return client;
} 

static void destroy_remote_client(remote_client_t* client)
{
    switch (client->type)
    {
    case PROTOCAL_TCP:
        //destroy_tcp_client(client->clt);
        break;

    case PROTOCAL_MQTT:
        printf("Error: MQTT protocal is not supproted\n");
        break;

    default:
        printf("Error: unkonwn protocal\n");
        break;
    }

    handle_free(client->handle);
    free(client);
}


client_t connect_to_server(client_protocal_type type, const char* ip, uint16_t port)
{
    client_t ret = -1;
    remote_client_t* client = NULL;

    client = get_client_by_ip_port(ip, port);
    if( client == NULL ) {
        client = create_remote_client(type, ip, port);
        if( client != NULL ) {
            ret = connect_to_remote_server(client);
            if( ret == CLIENT_ERR_SUCCESS ) {
                ret = launch_main_loop(client);
                if( ret == CLIENT_ERR_SUCCESS ) {
                    DL_APPEND(client_list, client);
                    ret = client->handle;
                } else {
                    destroy_remote_client(client);
                    ret = INVALID_HANDLE;
                }
            } else {
                destroy_remote_client(client);
                ret = INVALID_HANDLE;
            }
        } else {
            printf("Error: Fail to create client instance\n");
            ret = INVALID_HANDLE;
        }
    } else {
        printf("Found existed instance %d\n", client->handle);
        ret = client->handle;
    }

    return ret;
}

client_err_t disconnect_from_server(client_t handle)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    remote_client_t* client = NULL;

    client = get_client_handle(handle);
    if( client != NULL ) {
        disconnect_remote_server(client);
        stop_main_loop(client);
        destroy_remote_client(client);
        /*TODO: remove form list*/
    } else {
        ret = CLIENT_ERR_INVALID_PARAM;
    }

    return ret;    
}

client_err_t login_to_server(client_t handle)
{

}

client_err_t logout_from_server(client_t handle)
{

}

client_err_t get_login_status(client_t handle, bool* login_status)
{

}

client_err_t client_data_upload(client_t handle, const void* payload, int len)
{


}

