#include "mqtt_client.h"

#define MQTT_KEEPALIVE_PERIOD 60

static void* mqtt_client_main_loop(void *obj);

#if 0
static pthread_mutex_t mid_mutex = PTHREAD_MUTEX_INITIALIZER;

static uint16_t generate_mid()
{
	static uint16_t mid = 0;

	pthread_mutex_lock(&mid_mutex);
	if( ++mid == 0 ) {
        ++mid;
    }    
	pthread_mutex_unlock(&mid_mutex);

	return mid;
}
#endif

static void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    if( rc == 0 ) {
        printf("connect to mqtt broker successfully\n");
        /* TODO: do subscribe */
    }

} 

static void on_disconect(struct mosquitto *mosq, void *obj, int rc)
{
    printf("disconnect from mqtt broker\n");
    /* Publish disconnect event */
}

static void on_publish(struct mosquitto *mosq, void *obj, int mid)
{
    printf("on publish mid = %d\n", mid);
}

static void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{


}

static void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{


}


void* create_mqtt_client(remote_client_t *client)
{
    mqtt_client *clt = NULL;

    clt = malloc(sizeof(mqtt_client));
    if( !clt ) {
        return clt;
    }

    clt->mosq =  mosquitto_new(NULL, true, (void *)client);
    if( !clt ) {
        free(clt);
        clt = NULL;
        return clt;
    }

    //clt->out_msg_table = NULL;
    clt->client_thread_id = INVALID_THREAD;
    //pthread_mutex_init(&clt->out_msg_mutex, NULL);
    mosquitto_connect_callback_set(clt->mosq, &on_connect);
    mosquitto_disconnect_callback_set(clt->mosq, &on_disconect);
    mosquitto_publish_callback_set(clt->mosq, &on_publish);
    mosquitto_subscribe_callback_set(clt->mosq, &on_subscribe);
    mosquitto_message_callback_set(clt->mosq, &on_message);

    return clt;
}

void destroy_mqtt_client(remote_client_t* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    mqtt_client *clt = NULL;

    if( !client ) {
        return ;
    }

    clt = (mqtt_client *)(client->clt);
    if( !clt ) {
        return ;
    }

    mosquitto_destroy( clt->mosq );

    clt->mosq = NULL;

    free(clt);   
}

client_err_t connect_mqtt_server(remote_client_t* client, const connect_opt* opt)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    mqtt_client *clt = NULL;
    int result = 0;

    if( !client ) {
        return CLIENT_ERR_INVALID_PARAM;
    }

    clt = (mqtt_client *)(client->clt);
    if( !clt ) {
        return CLIENT_ERR_INVALID_PARAM;
    }

    result = mosquitto_connect(clt->mosq, opt->host, opt->port, MQTT_KEEPALIVE_PERIOD);
    if( result == MOSQ_ERR_INVAL ) {
        ret = CLIENT_ERR_INVALID_PARAM;
    } else if( result == MOSQ_ERR_ERRNO ){
        ret = CLIENT_ERR_ERRNO;        
    }

    return ret;
}

client_err_t disconnect_mqtt_server(remote_client_t* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    mqtt_client *clt = NULL;
    int result = 0;

    if( !client ) {
        return CLIENT_ERR_INVALID_PARAM;
    }

    clt = (mqtt_client *)(client->clt);
    if( !clt ) {
        return CLIENT_ERR_INVALID_PARAM;
    }

    result = mosquitto_disconnect(clt->mosq);
    if( result == MOSQ_ERR_INVAL ) {
        ret = CLIENT_ERR_INVALID_PARAM;
    } else if( result == MOSQ_ERR_NO_CONN ){
        ret = CLIENT_ERR_NO_CONN;        
    }

    return ret;
}

client_err_t start_mqtt_main_loop(remote_client_t* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    mqtt_client *clt = NULL;

    if( !client ) {
        return CLIENT_ERR_INVALID_PARAM;
    }

    clt = (mqtt_client *)(client->clt);
    if( !clt ) {
        return CLIENT_ERR_INVALID_PARAM;
    }

    mosquitto_threaded_set(clt->mosq, true);

    if( pthread_create(&clt->client_thread_id, NULL, mqtt_client_main_loop, (void *)client) != 0 ) {
        mosquitto_threaded_set(clt->mosq, false);
        ret = CLIENT_ERR_ERRNO;
    }

    return ret;
}

void stop_mqtt_main_loop(remote_client_t* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    mqtt_client *clt = NULL;

    if( !client ) {
        return ;
    }

    clt = (mqtt_client *)(client->clt);
    if( !clt ) {
        return ;
    }

    if( clt->client_thread_id != INVALID_THREAD ) {
        printf("cancel the thread [%ld]\n", (long unsigned int)(clt->client_thread_id));
        pthread_cancel(clt->client_thread_id);
        pthread_join(clt->client_thread_id, NULL);
        clt->client_thread_id = INVALID_THREAD;
    }

}


client_err_t mqtt_data_upload(remote_client_t *client, const void *payload, int len)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    mqtt_client *clt = NULL;
    mqtt_msg *msg = NULL;
    int result = 0;

    if( !client || !payload ) {
        return CLIENT_ERR_INVALID_PARAM;
    }

    msg = (mqtt_msg *)payload;
    clt = (mqtt_client *)(client->clt);
    if( !clt ) {
        return CLIENT_ERR_INVALID_PARAM;
    }

    result = mosquitto_publish(clt->mosq, msg->mid, msg->topic, msg->payload_len, msg->payload, msg->qos, msg->retain);

    switch (result) {
        case MOSQ_ERR_INVAL:
        case MOSQ_ERR_PROTOCOL:
        case MOSQ_ERR_PAYLOAD_SIZE:
        case MOSQ_ERR_MALFORMED_UTF8:
        case MOSQ_ERR_QOS_NOT_SUPPORTED:
        case MOSQ_ERR_OVERSIZE_PACKET:
            result = CLIENT_ERR_INVALID_PARAM;
            break;

        case MOSQ_ERR_NOMEM:
            result = CLIENT_ERR_NOMEN;

        case MOSQ_ERR_NO_CONN:  
            result = CLIENT_ERR_NO_CONN;

        default:
            break;
    }

    return result;
}


static void* mqtt_client_main_loop(void* obj)
{
    int run = 1;
    int rc = 0;
    remote_client_t *client = NULL;
    mqtt_client *clt = NULL;

    client = (remote_client_t *)obj;
    if( !client ) {
        return (void *)CLIENT_ERR_INVALID_PARAM;
    }

    clt = (mqtt_client *)(client->clt);
    if( !clt ) {
        return (void *)CLIENT_ERR_INVALID_PARAM;
    }

    printf("mqtt_client_main_loop start\n");
    while( run ) {

        pthread_testcancel();

        rc = mosquitto_loop(clt->mosq, -1, 1);

        switch (rc) {
            case MOSQ_ERR_NOMEM:
			case MOSQ_ERR_PROTOCOL:
			case MOSQ_ERR_INVAL:
			case MOSQ_ERR_NOT_FOUND:
			case MOSQ_ERR_TLS:
			case MOSQ_ERR_PAYLOAD_SIZE:
			case MOSQ_ERR_NOT_SUPPORTED:
			case MOSQ_ERR_AUTH:
			case MOSQ_ERR_ACL_DENIED:
			case MOSQ_ERR_UNKNOWN:
			case MOSQ_ERR_EAI:
			case MOSQ_ERR_PROXY:
			case MOSQ_ERR_ERRNO:
                run = 0;
				break;
        
            default:
                break;
        }

        if( errno == EPROTO ) {
            run = 0;
        }
    }

    return (void *)0;
}