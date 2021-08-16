#include <sys/socket.h>
#include "tcp_client.h"
#include "message_handler.h"


client_err_t start_tcp_main_loop(remote_client_t* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    tcp_client *clt = NULL;

    if( client != NULL ) {
        clt = (tcp_client *)(client->clt);
        if( clt != NULL ) {
            if( pthread_create(&clt->client_thread_id, NULL, tcp_client_main_loop, (void *)client) == 0 ) {
                printf("tcp_client_main_loop [%ld] created\n", (long unsigned int)(clt->client_thread_id));
            } else {
                printf("Fatal: tcp_client_main_loop thread create failed\n");
                ret = CLIENT_ERR_ERRNO;
            }
        } else {
           ret = CLIENT_ERR_INVALID_PARAM; 
        }    
    } else {
        ret = CLIENT_ERR_INVALID_PARAM;
    }     

    return ret;
}

void stop_tcp_main_loop(remote_client_t* client)
{
    tcp_client *clt = NULL;

    if( client != NULL ) {
        clt = (tcp_client *)(client->clt);
        if( clt != NULL ) {
            if( clt != NULL && clt->client_thread_id != INVALID_THREAD ) {
                if( clt->sockpair_w != INVALID_SOCKET ) {
                    send_internal_signal(clt->sockpair_w, internal_cmd_break_block);
                }

                printf("cancel the thread [%ld]\n", (long unsigned int)(clt->client_thread_id));
                pthread_cancel(clt->client_thread_id);
                pthread_join(clt->client_thread_id, NULL);
                clt->client_thread_id = INVALID_THREAD;
            }
        }    
    }    

    return ;
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
        client->client_thread_id = INVALID_THREAD;
        client->out_packet = NULL;
        client->out_packet_last = NULL;
        packet_init(&client->in_packet);
        pthread_mutex_init(&client->state_mutex, NULL);
        pthread_mutex_init(&client->out_packet_mutex, NULL);

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

void destroy_tcp_client(remote_client_t* client)
{
    tcp_client *clt = NULL;

    if( client != NULL ) {
        clt = (tcp_client *)(client->clt);
        if( clt != NULL ) {
            if( clt->sockfd != INVALID_SOCKET ) {
                net_socket_close(clt);
            }

            if( clt->sockpair_r != INVALID_SOCKET ) {
                close(clt->sockpair_r);
            }

            if( clt->sockpair_w != INVALID_SOCKET ) {
                close(clt->sockpair_w);
            }

            packet_cleanup_all(clt);

            free(clt->host);

            free(clt);
        }    
    }
}

client_err_t connect_tcp_server(remote_client_t* client, const connect_opt* opt)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    tcp_client *clt = NULL;

    if( client != NULL ) {
        clt = (tcp_client *)(client->clt);
        if( clt != NULL ) {
            tcp_client_state state = client_get_state(clt); 
            if( state == tcp_cs_new ) {
                clt->host = strdup(opt->host);
                if(!clt->host) {
                    return CLIENT_ERR_NOMEN;
                }
                clt->port = opt->port;

                ret = connect_server(clt);
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
            printf("Invalid tcp client, can't connect\n");
        }
    } else {
        ret = CLIENT_ERR_INVALID_PARAM;
        printf("Invalid instance, can't connect\n");
    }

    return ret;
}

client_err_t disconnect_tcp_server(remote_client_t* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    tcp_client *clt = NULL;

    if( client != NULL ) {
        clt = (tcp_client *)(client->clt);
        if( clt != NULL ) {
            if( clt->sockpair_w != INVALID_SOCKET ) {
                send_internal_signal(clt->sockpair_w, internal_cmd_disconnect);
            }
        } else {
            ret = CLIENT_ERR_INVALID_PARAM;
            printf("Invalid tcp client, can't disconnect\n");   
        }   
    } else {
        ret = CLIENT_ERR_INVALID_PARAM;
        printf("Invalid instance, can't disconnect\n");   
    }
    
    return ret;
}

#if 0
client_err_t get_tcp_login_status(tcp_client* client, bool* login_status)
{

}

client_err_t register_to_tcp_server(tcp_client* client)
{

}

client_err_t unregister_from_tcp_server(tcp_client* client)
{

}
#endif

client_err_t tcp_client_data_upload(remote_client_t* client, const void* payload, int len)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    tcp_packet_t* packet = NULL;
    tcp_client_state state;
    tcp_client *clt = NULL;

    if( client != NULL ) {
        clt = (tcp_client *)(client->clt);
        if( clt != NULL ) {
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
            printf("Invalid tcp client, can't upload data\n");
            return CLIENT_ERR_INVALID_PARAM;   
        }
    } else {
        printf("Invalid instance, can't upload data\n");
        return CLIENT_ERR_INVALID_PARAM;   
    }
    
    return packet_queue(client, packet);
}
