#include "client_api.h"
#include "tcp_client.h"

static remote_client_t* client_list = NULL;
static uint32_t fd;

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

static client_err_t start_remote_client_loop(remote_client_t* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    switch (client->type)
    {
    case PROTOCAL_TCP:
        ret = start_tcp_main_loop(client);
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

static void stop_remote_client_loop(remote_client_t* client)
{
    switch (client->type)
    {
    case PROTOCAL_TCP:
        stop_tcp_main_loop(client);
        break;

    case PROTOCAL_MQTT:
        printf("Error: MQTT protocal is not supproted\n");
        break;

    default:
        printf("Error: unkonwn protocal\n");
        break;
    }

    return ;
}

static client_t handle_alloc(void* p)
{
    return (client_t)p;
}

static void handle_free()
{


}

static client_err_t connect_to_remote_server(remote_client_t* client, const connect_opt* opt)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    switch (client->type)
    {
    case PROTOCAL_TCP:
        ret = connect_tcp_server(client, opt);
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
        ret = disconnect_tcp_server(client);
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


static remote_client_t* create_remote_client(client_protocal_type type)
{
    remote_client_t* client = NULL;

    client = (remote_client_t *)calloc(1, sizeof(remote_client_t));
    if( client != NULL ) {
        client->handle = handle_alloc(client);
        if( client->handle != INVALID_HANDLE ) {
            client->type = type;
            client->msg_thread_id = INVALID_THREAD;
            client->in_msg = NULL;
            pthread_mutex_init(&client->in_msg_mutex, NULL);
            sem_init(&client->in_msg_sem, 0, 0);

            switch (type) {
            case PROTOCAL_TCP:
                client->clt = create_tcp_client();
                if( client->clt == NULL ) {
                    handle_free(client->handle);
                    free(client);
                    client = NULL;    
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
        destroy_tcp_client(client);
        client->clt = NULL;
        break;

    case PROTOCAL_MQTT:
        printf("Error: MQTT protocal is not supproted\n");
        break;

    default:
        printf("Error: unkonwn protocal\n");
        break;
    }

    message_cleanup_all(client);
    handle_free(client->handle);
    free(client);
}

static client_err_t data_upload(remote_client_t* client, const void* payload, int len)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    switch (client->type)
    {
    case PROTOCAL_TCP:
        ret = tcp_client_data_upload(client, payload, len);
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

client_t connect_to_server(const connect_opt* opt)
{
    client_t ret = -1;
    remote_client_t* client = NULL;

    client = create_remote_client(opt->type);
    if( client != NULL ) {
        if( connect_to_remote_server(client, opt) == CLIENT_ERR_SUCCESS ) {
            if( start_remote_client_loop(client) ==  CLIENT_ERR_SUCCESS ) { 
                if( start_message_handle_loop(client) == CLIENT_ERR_SUCCESS ) {
                    DL_APPEND(client_list, client);
                    ret = client->handle;
                } else {
                    disconnect_remote_server(client);
                    stop_remote_client_loop(client);
                    destroy_remote_client(client);
                    client = NULL;
                    ret = INVALID_HANDLE;
                }
            } else {
                disconnect_remote_server(client);
                destroy_remote_client(client);
                client = NULL;
                ret = INVALID_HANDLE;
            }
        } else {
            destroy_remote_client(client);
            client = NULL;
            ret = INVALID_HANDLE;
        }
    } else {
        printf("Error: Fail to create client instance\n");
        ret = INVALID_HANDLE;
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
        stop_remote_client_loop(client);
        stop_message_handle_loop(client);
        DL_DELETE(client_list, client);
        destroy_remote_client(client);
        client = NULL;
    } else {
        ret = CLIENT_ERR_INVALID_PARAM;
    }

    return ret;    
}


client_err_t client_data_upload(client_t handle, const void* payload, int len)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    remote_client_t* client = NULL;

    client = get_client_handle(handle);
    if( client != NULL ) {
        ret = data_upload(client, payload, len);
    } else {
        ret = CLIENT_ERR_INVALID_PARAM;
    }

    return ret; 

}

