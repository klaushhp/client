#include "tcp_packet.h"
#include <sys/socket.h>

static ssize_t net_write(tcp_client* client, void* buf, size_t count);
static ssize_t net_read(tcp_client* client, void* buf, size_t count);


void packet_init(tcp_packet_t* packet)
{
    if( packet != NULL ) {
        packet->payload = NULL;
        packet->payload_len = 0;;
        packet->next = NULL;
        packet->pos = 0;
        packet->to_process = 0;  

        memset(packet->head_buf, 0, sizeof(packet->head_buf));
    }
}

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

    pthread_mutex_lock(&client->out_packet_mutex);

    while( client->out_packet ) {
        packet = client->out_packet;
        client->out_packet = client->out_packet->next;
        packet_cleanup(packet);
        packet_free(packet);
    }

    client->out_packet = NULL;
    client->out_packet_last = NULL;

    pthread_mutex_unlock(&client->out_packet_mutex);
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

client_err_t packet_queue(remote_client_t* client, tcp_packet_t* packet)
{
    tcp_client *clt = NULL;

    if( client == NULL || packet == NULL ) {
        return CLIENT_ERR_INVALID_PARAM;
    } 

    clt = (tcp_client *)(client->clt);
    if( clt == NULL ) {
        CLIENT_ERR_INVALID_PARAM;
    }

    packet->next = NULL;
    pthread_mutex_lock(&clt->out_packet_mutex);
    if(clt->out_packet) {
        clt->out_packet_last->next = packet;
    } else {
        clt->out_packet= packet;
    }
    clt->out_packet_last = packet;
    pthread_mutex_unlock(&clt->out_packet_mutex);

    if(clt->sockpair_w != INVALID_SOCKET) {
        send_internal_signal(clt->sockpair_w, internal_cmd_write_trigger);
    }
 
    return CLIENT_ERR_SUCCESS;
}

client_err_t packet_write(remote_client_t* client)
{
    tcp_packet_t* packet = NULL;
    ssize_t write_len;
    tcp_client_state state;
    tcp_client *clt = NULL;

    if( client == NULL ) {
        return CLIENT_ERR_INVALID_PARAM;
    }

    clt = (tcp_client *)(client->clt);
    if( clt == NULL ) {
        return CLIENT_ERR_INVALID_PARAM;
    }

    while(clt->out_packet) {
        packet = clt->out_packet;

        write_len = net_write(clt, packet->payload, packet->payload_len);
        if(write_len > 0) {
            printf("successfully send [%ld] bytes: %s\n", write_len, (char*)packet->payload);
        } else {
            printf("ret:[%ld] fail to send\n", write_len);
            if(errno == EAGAIN || errno == EWOULDBLOCK ) {
				return CLIENT_ERR_SUCCESS;
			} else {
				switch(errno) {
				case ECONNRESET:
                    return CLIENT_ERR_CONN_LOST;
            
                default:
                    return CLIENT_ERR_ERRNO;
				}
			}
        }

        pthread_mutex_lock(&clt->out_packet_mutex);
        clt->out_packet = clt->out_packet->next;
        if(!clt->out_packet) {
            clt->out_packet_last = NULL;
        }
        pthread_mutex_unlock(&clt->out_packet_mutex);

        packet_cleanup(packet);
        packet_free(packet);
    }

    return CLIENT_ERR_SUCCESS;
}

static bool bcc_check_sum(const tcp_packet_t* packet, uint8_t check_sum_old)
{
    int i;
    uint8_t check_sum = 0;

    check_sum = packet->head_buf[2];
    for(i=3; i<MSG_BEFORE_DATA_LEN; i++)
    {
        check_sum ^= packet->head_buf[i];
    }

    for(i=0; i<packet->payload_len; i++)
    {
        check_sum ^= ((uint8_t *)packet->payload)[i];
    }

    printf("old sum: [0x%x] current sum[0x%x]\n", check_sum_old, check_sum);

    return (check_sum == check_sum_old);
}

static void print_msg(const uint8_t* msg, uint16_t len)
{
    int i;

    if( msg != NULL && len > 0 ) {
        for(i=0; i<len; i++)
        {
            printf("0x%x ", msg[i]);

        }

        printf("\n");
    }
}


client_err_t packet_read(remote_client_t* client)
{
    uint8_t byte = 0;
    uint8_t check_sum = 0;
    int rc = 0;
    tcp_client *clt = NULL;

    if( !client )
    {
        return CLIENT_ERR_INVALID_PARAM;
    }

    clt = (tcp_client *)(client->clt);
    if( !clt )
    {
        return CLIENT_ERR_INVALID_PARAM;
    }


    while(clt->in_packet.pos < 2 ) {
        rc = net_read(clt, &byte, 1);
        if( rc == 1 ) {
            if( byte == MSG_START_ID ) {
                clt->in_packet.pos++;
                if( clt->in_packet.pos == 2 ) {
                    clt->in_packet.head_buf[0] = MSG_START_ID;
                    clt->in_packet.head_buf[1] = MSG_START_ID;
                    printf("msg found--------\n");
                }
            } else {
                clt->in_packet.pos = 0;
                printf("ignore [0x%x]\n", byte);
            }
        } else {
            if( rc == 0) {
                printf("warnning: client lost connection!\n");
                return CLIENT_ERR_CONN_LOST;
            } else {
                goto err;
            }
        }    
    }

    while( clt->in_packet.pos < MSG_BEFORE_DATA_LEN ) {
        rc = net_read(clt, clt->in_packet.head_buf + clt->in_packet.pos, MSG_BEFORE_DATA_LEN - clt->in_packet.pos);
        if( rc > 0 ) {
            clt->in_packet.pos += rc;
            if( clt->in_packet.pos == MSG_BEFORE_DATA_LEN ) {
                memcpy(&clt->in_packet.payload_len, clt->in_packet.head_buf + MSG_BEFORE_DATA_LEN - MSG_DATA_LEN, MSG_DATA_LEN);
                printf("payload len = %d\n", clt->in_packet.payload_len);
                clt->in_packet.payload = malloc(clt->in_packet.payload_len);
                if( clt->in_packet.payload == NULL ) {
                    return CLIENT_ERR_NOMEN;
                    /*TODO: clean pkg*/
                }
                clt->in_packet.to_process = clt->in_packet.payload_len;
            } 
        } else {
            goto err;
        }
    }

    while( clt->in_packet.to_process > 0 ) {
        rc = net_read(clt, clt->in_packet.payload + clt->in_packet.pos - MSG_BEFORE_DATA_LEN, clt->in_packet.to_process);
        if( rc > 0 ) {
            clt->in_packet.to_process -= rc;
            clt->in_packet.pos += rc;
        } else {
            goto err;
        }
    }

    rc = net_read(clt, &check_sum, MSG_CHECK_SUM_LEN);
    if( rc < 0) {
        goto err;
    }

    print_msg(clt->in_packet.head_buf, 12);
    print_msg(clt->in_packet.payload, clt->in_packet.payload_len);
    print_msg(&check_sum, 1);

    if( bcc_check_sum(&clt->in_packet, check_sum) ) {
        pending_msg_t* msg = calloc(1, sizeof(pending_msg_t));
        if( msg == NULL ) {
            return CLIENT_ERR_NOMEN;
        }  

        msg->payload = clt->in_packet.payload;
        msg->payload_len = clt->in_packet.payload_len;
        msg->check_sum = check_sum;
        memcpy(&msg->cmd_id, clt->in_packet.head_buf + MSG_START_ID_LEN, sizeof(msg->cmd_id));
        memcpy(&msg->ack_flag, clt->in_packet.head_buf + MSG_START_ID_LEN + MSG_CMD_ID_LEN, sizeof(msg->ack_flag));
        memcpy(msg->date, clt->in_packet.head_buf + MSG_START_ID_LEN + MSG_CMD_ID_LEN + MSG_ACK_FLAG_LEN, sizeof(msg->date));
        memcpy(msg->time, clt->in_packet.head_buf + MSG_START_ID_LEN + MSG_CMD_ID_LEN + MSG_ACK_FLAG_LEN + MSG_DATE_LEN, sizeof(msg->time));
    
        pthread_mutex_lock(&client->in_msg_mutex);
        DL_APPEND(client->in_msg, msg);
        pthread_mutex_unlock(&client->in_msg_mutex);
    
        sem_post(&client->in_msg_sem);

        packet_init(&clt->in_packet);
        
        return CLIENT_ERR_SUCCESS;
    }

err:
    if( rc == 0) {
        printf("warnning: client lost connection!\n");
        return CLIENT_ERR_CONN_LOST;
    } else {
        printf("%s\n", strerror(errno));
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            return CLIENT_ERR_SUCCESS;
        } else {
            switch (errno) {
            case ECONNRESET:
                return CLIENT_ERR_CONN_LOST;
            
            default:
                 return CLIENT_ERR_ERRNO;
            }
        }  
    }

}


static ssize_t net_write(tcp_client* client, void* buf, size_t count)
{
#ifdef ENABLE_TLS

#endif

    errno = 0;
#ifdef ENABLE_TLS

#endif
    return send(client->sockfd, buf, count, 0);
}

static ssize_t net_read(tcp_client* client, void* buf, size_t count)
{
#ifdef ENABLE_TLS

#endif 

    errno = 0;
#ifdef ENABLE_TLS

#endif

    return recv(client->sockfd, buf, count, 0);
}
