#include <sys/socket.h>
#include "tcp_client.h"

static bool check_client_validity(net_client* client)
{
    return ( (client != NULL) && (client->tcp_clt != NULL) );
}

static client_err_t launch_tcp_main_loop(net_client* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
        
    if( pthread_create(&client->tcp_clt->thread_id, NULL, tcp_client_main_loop, (void *)client) != 0 )
    {
        ret = CLIENT_ERR_ERRNO;
    } 
        
    return ret;
}

static bool tcp_client_init(tcp_client** tcp_clt, const char* name)
{
    bool ret = true;
    tcp_client* clt = NULL;

    *tcp_clt = (tcp_client *)calloc(1, sizeof(tcp_client));
    clt = *tcp_clt;
    if( clt != NULL ) {
        strncpy(clt->client_name, name, sizeof(clt->client_name));
        memset(clt->address, 0, sizeof(clt->address));
        clt->sockfd = INVALID_SOCKET;
        clt->port = 0;
        clt->state = tcp_cs_new;
        clt->out_packet = NULL;
        clt->out_packet_last = NULL;

        pthread_mutex_init(&clt->state_mutex, NULL);
        pthread_mutex_init(&clt->packet_mutex, NULL);

        if( local_socketpair(&clt->sockpair_r, &clt->sockpair_w) ) {
            ret = false;
            free(clt);
        }
    } else {
        ret = false;
    }   

    return ret;
}

net_client* create_tcp_client(const char* client_name)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    net_client* client = NULL;

    client = (net_client *)calloc(1, sizeof(net_client));
    if( client != NULL )
    {
        if( tcp_client_init(&client->tcp_clt, client_name) ) {
            client->login_status = false;
            pthread_mutex_init(&client->login_mutex, NULL);
        } else {
            free(client);
        }
    }    
 
    return client;
}

void destory_tcp_client(net_client* client)
{
    /*interrupt sleep*/
    /*TODO: do disconnect*/ 
    /*TODO: cancel loop thread*/
    /*TODO: release resource*/

}

client_err_t connect_tcp_server(net_client* client, const char* ip, uint16_t port)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    tcp_client* clt = NULL;

    if( check_client_validity(client) ) {
        clt = client->tcp_clt;
        tcp_client_state state = client_get_state(clt); 
        if( state == tcp_cs_new || state == tcp_cs_disconnected ) {
            strncpy(clt->address, ip, sizeof(clt->address));
            clt->port = port;

            ret = connect_server(clt);
            if(  ret == CLIENT_ERR_SUCCESS ) {
                ret = launch_tcp_main_loop(client);
                if( ret == CLIENT_ERR_SUCCESS ) {
                    /* publish connect success */
                    printf("publish event: connect success！\n");
                } else {
                    close(clt->sockfd);
                }
            }
        } else {
            ret = CLIENT_ERR_INVALID_OPERATION;
            printf("Invalid status, can't connect, client status[%d]\n", state);
        }
    } else {
        ret = CLIENT_ERR_INVALID_PARAM;
        printf("Invalid instance, can't connect\n");
    }

    if( ret != CLIENT_ERR_SUCCESS ) {
        client_set_state(clt, tcp_cs_new);
    }

    return ret;
}

client_err_t disconnect_tcp_server(net_client* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    tcp_client* clt = NULL;

    if( check_client_validity(client) ) {
        clt = client->tcp_clt;

        if( clt->sockpair_w != INVALID_SOCKET ) {
            send_internal_signal(clt->sockpair_w, internal_cmd_disconnect);
        }    
    }
    else
    {
        ret = CLIENT_ERR_INVALID_PARAM;
        printf("Invalid instance, can't connect\n");   
    }
    
    return ret;
}

client_err_t get_tcp_login_status(net_client* client, bool* login_status)
{

}

client_err_t register_to_tcp_server(net_client* client)
{

}

client_err_t unregister_from_tcp_server(net_client* client)
{

}

client_err_t tcp_client_data_upload(net_client* client, const void* payload, int len)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    tcp_client* clt = NULL;
    tcp_packet_t* packet = NULL;
    tcp_client_state state;

    if( check_client_validity(client) ) {
        clt = client->tcp_clt;

        state = client_get_state(clt); 
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
    
    return packet_queue(clt, packet);
}

#if 0
void tcp_loop_stop(tcp_client* client)
{
    pthread_cancel(client->thread_id);
    pthread_join(client->thread_id, NULL);
}
#endif