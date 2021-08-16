#include "message_handler.h"

static void hande_query_param(pending_msg_t* msg);
static void hande_set_param(pending_msg_t* msg);
static void hande_vehicle_control(pending_msg_t* msg);
static void hande_request_can_msg(pending_msg_t* msg);
static void *message_handle_loop(void *obj);


static void message_cleanup(pending_msg_t* msg)
{
    if( msg != NULL ) {
        msg->payload_len = 0;
        free(msg->payload);
        msg->payload = NULL;
    }    
}

static void message_free(pending_msg_t *msg)
{
    free(msg);
}

void message_cleanup_all(remote_client_t *client)
{
    pending_msg_t *current, *tmp;

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

client_err_t start_message_handle_loop(remote_client_t* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    if( pthread_create(&client->msg_thread_id, NULL, message_handle_loop, (void *)client) == 0 ) {
        printf("tcp_message_handle_loop [%ld] created\n", (long unsigned int)client->msg_thread_id);
    } else {
        printf("Fatal: tcp_message_handle_loop thread create failed\n");
        ret = CLIENT_ERR_ERRNO;
    }

    return ret;
}

void stop_message_handle_loop(remote_client_t *client)
{
    if( client != NULL )
    {
        sem_post(&client->in_msg_sem);
        printf("cancel the thread [%ld]\n", (long unsigned int)client->msg_thread_id);
        pthread_cancel(client->msg_thread_id);
        pthread_join(client->msg_thread_id, NULL);
        client->msg_thread_id = INVALID_THREAD;
    }
  
    return ;
}

static void* message_handle_loop(void *obj)
{
    pending_msg_t *current = NULL;
    pending_msg_t *tmp = NULL;
    remote_client_t *client = (remote_client_t *)obj;

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

static void hande_query_param(pending_msg_t* msg)
{

}

static void hande_set_param(pending_msg_t* msg)
{

}

static void hande_vehicle_control(pending_msg_t* msg)
{

}

static void hande_request_can_msg(pending_msg_t* msg)
{


}