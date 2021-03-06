#include <sys/socket.h>
#include <time.h>
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
    client_internal_cmd cmd = internal_cmd_break_block;

    if( client != NULL ) {
        clt = (tcp_client *)(client->clt);
        if( clt != NULL ) {
            if( clt != NULL && clt->client_thread_id != INVALID_THREAD ) {
                if( clt->sockpair_w != INVALID_SOCKET ) {
                    send(clt->sockpair_w, &cmd, 1, 0);
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

void* create_tcp_client(remote_client_t* client)
{
    (void)client;
    client_err_t ret = CLIENT_ERR_SUCCESS;
    tcp_client* clt = NULL;

    clt = (tcp_client *)malloc(sizeof(tcp_client));
    if( clt != NULL ) {
        clt->sockfd = INVALID_SOCKET;
        clt->host = NULL;
        clt->port = 0;
        clt->state = tcp_cs_new;
        clt->client_thread_id = INVALID_THREAD;
        clt->out_packet = NULL;
        clt->out_packet_last = NULL;
        clt->current_out_packet = NULL;
        packet_init(&clt->in_packet);
        pthread_mutex_init(&clt->state_mutex, NULL);
        pthread_mutex_init(&clt->out_packet_mutex, NULL);
        pthread_mutex_init(&clt->current_out_packet_mutex, NULL);

        if( local_socketpair(&clt->sockpair_r, &clt->sockpair_w, false) ) {
            printf("Error: fail to create internal pipe\n");
            free(client);
            client = NULL;
        }
    } else {
        printf("Error: fail to alloc tcp_client instance\n");
    }   

    return clt;
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
    client_internal_cmd cmd = internal_cmd_disconnect;

    if( client != NULL ) {
        clt = (tcp_client *)(client->clt);
        if( clt != NULL ) {
            if( clt->sockpair_w != INVALID_SOCKET ) {
                send(clt->sockpair_w, &cmd, 1, 0);
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


static client_err_t wait_upload_result(tcp_packet_t* packet)
{
    int read_byte;
    client_err_t ret = CLIENT_ERR_SUCCESS;
    uint8_t pairbuf; 
    struct timeval tv_out;
    tv_out.tv_sec = 1;
    tv_out.tv_usec = 0;
 
    setsockopt(packet->sockpair_r, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
    read_byte = recv(packet->sockpair_r, &pairbuf, 1, 0);
    if( read_byte == 1 ) {
        ret = (client_err_t)pairbuf;
        printf("send ret [%d]\n", ret);
    } else {
        printf("send wait time out: [%s]\n", strerror(errno));
        ret = CLIENT_ERR_ERRNO;
    }

    close(packet->sockpair_r);
    close(packet->sockpair_w);

    return ret;
}

client_err_t tcp_data_upload(remote_client_t* client, const void* payload, int len)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    tcp_packet_t* packet = NULL;
    tcp_client_state state;
    tcp_client *clt = NULL;

    if( client != NULL ) {
        clt = (tcp_client *)(client->clt);
        if( clt != NULL ) {
            state = client_get_state(clt); 
            if( state == tcp_cs_connected ) {
                packet = calloc(1, sizeof(tcp_packet_t));
                if( !packet ) {
                    return CLIENT_ERR_NOMEN;
                }

                if( local_socketpair(&packet->sockpair_r, &packet->sockpair_w, true) ) {
                    printf("fail to create sockpair\n");
                    free(packet);
                    return CLIENT_ERR_ERRNO;
                }   

                packet->payload_len = len;
                ret = packet_alloc(packet);
                if( ret ) {
                    close(packet->sockpair_r);
                    close(packet->sockpair_w);
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
    
    packet_queue(client, packet);

    return wait_upload_result(packet);
}
