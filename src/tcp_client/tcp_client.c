#include <sys/socket.h>
#include "tcp_client.h"


client_err_t start_tcp_main_loop(tcp_client* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
        
    if( pthread_create(&client->thread_id, NULL, tcp_client_main_loop, (void *)client) != 0 )
    {
        ret = CLIENT_ERR_ERRNO;
    } 
    printf("thread [%ld] created\n", (long unsigned int)client->thread_id);

    return ret;
}

client_err_t stop_tcp_main_loop(tcp_client* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    void* rc;

    if( client != NULL )
    {
        if( client->sockpair_w != INVALID_SOCKET ) {
            send_internal_signal(client->sockpair_w, internal_cmd_break_block);
        }
        printf("cancel the thread [%ld]\n", (long unsigned int)client->thread_id);

        pthread_cancel(client->thread_id);
        pthread_join(client->thread_id, &rc);
        client->thread_id = INVALID_THREAD;
    }

    return ret;
}

tcp_client* create_tcp_client()
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    tcp_client* client = NULL;

    client = (tcp_client *)calloc(1, sizeof(tcp_client));
    if( client != NULL ) {
        client->sockfd = INVALID_SOCKET;
        client->host = NULL;
        client->port = 0;
        client->state = tcp_cs_new;
        client->thread_id = INVALID_THREAD;
        client->out_packet = NULL;
        client->out_packet_last = NULL;

        pthread_mutex_init(&client->state_mutex, NULL);
        pthread_mutex_init(&client->packet_mutex, NULL);

        if( local_socketpair(&client->sockpair_r, &client->sockpair_w) ) {
            printf("Error: fail to create internal pipe\n");
            free(client);
            client = NULL;
        }
    } else {
        printf("Error: fail to alloc tcp_client instance\n");
    }   

    return client;
}

void destroy_tcp_client(tcp_client* client)
{
    if( client != NULL ) {
        if( client->sockfd != INVALID_SOCKET ) {
            net_socket_close(client);
        }

        if( client->sockpair_r != INVALID_SOCKET ) {
            close(client->sockpair_r);
        }

        if( client->sockpair_w != INVALID_SOCKET ) {
            close(client->sockpair_w);
        }

        packet_cleanup_all(client);

        free(client->host);

        free(client);
    }
}

client_err_t connect_tcp_server(tcp_client* client, const char* host, uint16_t port)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    if( client != NULL ) {
        tcp_client_state state = client_get_state(client); 
        if( state == tcp_cs_new ) {
            client->host = strdup(host);
            if(!client->host) {
                return CLIENT_ERR_NOMEN;
            }
            client->port = port;

            ret = connect_server(client);
            if(  ret == CLIENT_ERR_SUCCESS ) {
                /* TODO: publish connect success */
                printf("publish event: connect success\n");
            }
        } else {
            ret = CLIENT_ERR_INVALID_OPERATION;
            printf("Invalid status, can't connect, client status[%d]\n", state);
        }
    } else {
        ret = CLIENT_ERR_INVALID_PARAM;
        printf("Invalid instance, can't connect\n");
    }

    return ret;
}

client_err_t disconnect_tcp_server(tcp_client* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    if( client != NULL ) {
        if( client->sockpair_w != INVALID_SOCKET ) {
            send_internal_signal(client->sockpair_w, internal_cmd_disconnect);
        }    
    }
    else
    {
        ret = CLIENT_ERR_INVALID_PARAM;
        printf("Invalid instance, can't disconnect\n");   
    }
    
    return ret;
}

client_err_t get_tcp_login_status(tcp_client* client, bool* login_status)
{

}

client_err_t register_to_tcp_server(tcp_client* client)
{

}

client_err_t unregister_from_tcp_server(tcp_client* client)
{

}

client_err_t tcp_client_data_upload(tcp_client* client, const void* payload, int len)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    tcp_packet_t* packet = NULL;
    tcp_client_state state;

    if( client != NULL ) {
        state = client_get_state(client); 
        if( state == tcp_cs_connected || state == tcp_cs_connect_pending ) {
            packet = calloc(1, sizeof(tcp_packet_t));
            if( !packet ) {
                return CLIENT_ERR_NOMEN;
            }

            packet->payload_len = len;
            ret = packet_alloc(packet);
            if( ret ) {
                free(packet);
                return ret;
            }

            if( packet->payload_len ) {
                memcpy(packet->payload, payload, packet->payload_len);
            }
        } else {
            printf("Invalid connect status, can't upload data\n");
            return CLIENT_ERR_INVALID_OPERATION;
        }
    } else {
        printf("Invalid instance, can't upload data\n");
        return CLIENT_ERR_INVALID_PARAM;   
    }
    
    return packet_queue(client, packet);
}
