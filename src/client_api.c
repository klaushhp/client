#include "client_api.h"
#include "tcp_client.h"
#include "mqtt_client.h"

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

static void set_client_callback(remote_client_t *client)
{
    switch (client->type)
    {
    case PROTOCAL_TCP:
        client->create_client = &create_tcp_client;
        client->destroy_client = &destroy_tcp_client;  
        client->connect_server = &connect_tcp_server;
        client->disconnect_server = &disconnect_tcp_server;
        client->start_main_loop = &start_tcp_main_loop;    
        client->stop_main_loop = &stop_tcp_main_loop;
        client->data_upload = &tcp_data_upload;

        break;

    case PROTOCAL_MQTT:
        client->create_client = &create_mqtt_client;
        client->destroy_client = &destroy_mqtt_client;  
        client->connect_server = &connect_mqtt_server;
        client->disconnect_server = &disconnect_mqtt_server;
        client->start_main_loop = &start_mqtt_main_loop;    
        client->stop_main_loop = &stop_mqtt_main_loop;
        client->data_upload = &mqtt_data_upload;
        break;

    default:
        printf("Error: unkonwn protocal\n");
        break;
    }

}


static client_t handle_alloc(void* p)
{
    return (client_t)p;
}

static void handle_free()
{


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
            set_client_callback(client);

            client->clt = client->create_client(client);
            if( client->clt == NULL ) {
                handle_free(client->handle);
                free(client);
                client = NULL;    
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
    client->destroy_client(client);
    client->clt = NULL;

    message_cleanup_all(client);
    handle_free(client->handle);
    free(client);
}

client_t connect_to_server(const connect_opt* opt)
{
    remote_client_t* client = NULL;

    client = create_remote_client(opt->type);
    if( !client ) {
        printf("Error: Fail to create client instance\n");
        return INVALID_HANDLE;
    }

    if( client->connect_server(client, opt) != CLIENT_ERR_SUCCESS ) {
        destroy_remote_client(client);
        client = NULL;
        return INVALID_HANDLE;
    }

    if( client->start_main_loop(client) !=  CLIENT_ERR_SUCCESS ) {
        client->disconnect_server(client);
        destroy_remote_client(client);
        client = NULL;
        return INVALID_HANDLE;
    }

    if( start_message_handle_loop(client) != CLIENT_ERR_SUCCESS) {
        client->disconnect_server(client);
        client->stop_main_loop(client);
        destroy_remote_client(client);
        client = NULL;
        return INVALID_HANDLE;
    }

    DL_APPEND(client_list, client);

    return client->handle;
}

client_err_t disconnect_from_server(client_t handle)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    remote_client_t* client = NULL;

    client = get_client_handle(handle);
    if( client != NULL ) {
        client->disconnect_server(client);
        client->stop_main_loop(client);
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
        ret = client->data_upload(client, payload, len);
    } else {
        ret = CLIENT_ERR_INVALID_PARAM;
    }

    return ret; 
}

