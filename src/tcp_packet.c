
#include "tcp_packet.h"

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
    if(client->msg) {
        client->msg->next = packet;
    } else {
        client->msg = packet;
    }
    pthread_mutex_unlock(&client->packet_mutex);

    if(client->sockpair_w != INVALID_SOCKET) {
        send_internal_signal(client->sockpair_w, internal_cmd_write_trigger);
    }
 
    return CLIENT_ERR_SUCCESS;
}
