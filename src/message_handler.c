#include "message_handler.h"

static void hande_query_param(tcp_packet_msg_t* msg);
static void hande_set_param(tcp_packet_msg_t* msg);
static void hande_vehicle_control(tcp_packet_msg_t* msg);
static void hande_request_can_msg(tcp_packet_msg_t* msg);

static void message_cleanup(tcp_packet_msg_t* msg)
{
    if( msg != NULL ) {
        msg->payload_len = 0;
        free(msg->payload);
        msg->payload = NULL;
    }    
}

static void message_free(tcp_packet_msg_t *msg)
{
    free(msg);
}

void message_cleanup_all(tcp_client *client)
{
    tcp_packet_msg_t *current, *tmp;

    if( client != NULL ) {
        pthread_mutex_lock(&client->in_msg_mutex);

        DL_FOREACH_SAFE(client->in_msg, current, tmp) {
            DL_DELETE(client->in_msg, current);
            message_cleanup(current);
            message_free(current);
        }

        pthread_mutex_unlock(&client->in_msg_mutex);

    }
}

void* tcp_message_handle_loop(void *obj)
{
    tcp_packet_msg_t *current = NULL;
    tcp_packet_msg_t *tmp = NULL;
    tcp_client* client = (tcp_client *)obj;

    do {
        sem_wait(&client->in_msg_sem);

        pthread_testcancel();

        pthread_mutex_lock(&client->in_msg_mutex);

        DL_FOREACH_SAFE(client->in_msg, current, tmp) {
            DL_DELETE(client->in_msg, current);
            break;
        }

        pthread_mutex_unlock(&client->in_msg_mutex);

        if( current != NULL ) {
            switch (current->cmd_id) {
            case CMD_ID_QUERY_PARAM:
                printf("Icoming cmd: CMD_ID_QUERY_PARAM\n");
                hande_query_param(current);
                break;

            case CMD_ID_SET_PARAM:
                printf("Icoming cmd: CMD_ID_SET_PARAM\n");
                hande_set_param(current);
                break;

            case CMD_ID_VEHICLE_CONTROL:
                printf("Icoming cmd: CMD_ID_VEHICLE_CONTROL\n");
                hande_vehicle_control(current);
                break;    
            
            case CMD_ID_REQUEST_CAN_MSG:
                printf("Icoming cmd: CMD_ID_REQUEST_CAN_MSG\n");
                hande_request_can_msg(current);
                break;
            default:
                printf("warnning: unknown cmd, can't handle\n");
                break;
            }

            message_cleanup(current);
            message_free(current);
            current = NULL;
        }

    } while(1);

}

static void hande_query_param(tcp_packet_msg_t* msg)
{

}

static void hande_set_param(tcp_packet_msg_t* msg)
{

}

static void hande_vehicle_control(tcp_packet_msg_t* msg)
{

}

static void hande_request_can_msg(tcp_packet_msg_t* msg)
{


}