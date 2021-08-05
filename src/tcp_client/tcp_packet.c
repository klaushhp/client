#include "tcp_packet.h"
#include <sys/socket.h>

void packet_cleanup(tcp_packet_t* packet)
{
    packet->payload_len = 0;
    free(packet->payload);
    packet->payload = NULL;
}

void packet_free(tcp_packet_t* packet)
{
    free(packet);
}

void packet_cleanup_all(tcp_client* client)
{
    tcp_packet_t* packet;

    pthread_mutex_lock(&client->packet_mutex);

    while( client->out_packet ) {
        packet = client->out_packet;
        client->out_packet = client->out_packet->next;
        packet_cleanup(packet);
        packet_free(packet);
    }

    client->out_packet = NULL;
    client->out_packet_last = NULL;

    pthread_mutex_unlock(&client->packet_mutex);
}

client_err_t packet_alloc(tcp_packet_t* packet)
{
    if( packet == NULL ) {
        return CLIENT_ERR_INVALID_PARAM;
    }

    packet->payload = malloc(sizeof(uint8_t)*packet->payload_len);
    if(!packet->payload_len) {
        return CLIENT_ERR_NOMEN;
    }

    return CLIENT_ERR_SUCCESS;
}

client_err_t packet_queue(tcp_client* client, tcp_packet_t* packet)
{
    if( client == NULL || packet == NULL ) {
        return CLIENT_ERR_INVALID_PARAM;
    } 

    packet->next = NULL;
    pthread_mutex_lock(&client->packet_mutex);
    if(client->out_packet) {
        client->out_packet_last->next = packet;
    } else {
        client->out_packet= packet;
    }
    client->out_packet_last = packet;
    pthread_mutex_unlock(&client->packet_mutex);

    if(client->sockpair_w != INVALID_SOCKET) {
        send_internal_signal(client->sockpair_w, internal_cmd_write_trigger);
    }
 
    return CLIENT_ERR_SUCCESS;
}

client_err_t packet_write(tcp_client* client)
{
    tcp_packet_t* packet = NULL;
    int write_len;
    tcp_client_state state;

    if( client == NULL ) {
        return CLIENT_ERR_INVALID_PARAM;
    }

    state = client_get_state(client);
    if( state == tcp_cs_connect_pending ) {
        return CLIENT_ERR_SUCCESS;
    }

    while(client->out_packet) {
        packet = client->out_packet;

        write_len = send(client->sockfd, packet->payload, packet->payload_len, 0);
        if(write_len > 0) {
            printf("successfully send [%d] bytes: %s\n", write_len, (char*)packet->payload);
        } else {
            printf("ret:%d fail to send\n", write_len);
            return CLIENT_ERR_ERRNO;
        }

        pthread_mutex_lock(&client->packet_mutex);
        client->out_packet = client->out_packet->next;
        if(!client->out_packet) {
            client->out_packet_last = NULL;
        }
        pthread_mutex_unlock(&client->packet_mutex);

        packet_cleanup(packet);
        packet_free(packet);
    }

    return CLIENT_ERR_SUCCESS;
}

client_err_t packet_read(tcp_client* client)
{
    char buf[1024] = {0};
    int rc;

    if( !client )
    {
        return CLIENT_ERR_INVALID_PARAM;
    }

    rc = recv(client->sockfd, buf, sizeof(buf), 0);
    if( rc == 0)
    {
        printf("warnning: client lost connection!\n");
        return CLIENT_ERR_CONN_LOST;
    } else if( rc == -1 )
    {
        printf("%s\n", strerror(errno));
        return CLIENT_ERR_ERRNO;
    }

    return CLIENT_ERR_SUCCESS;
}
