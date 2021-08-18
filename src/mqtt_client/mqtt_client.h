#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_

#include "common.h"


void* create_mqtt_client(remote_client_t *client);
void destroy_mqtt_client(remote_client_t *client);
client_err_t connect_mqtt_server(remote_client_t *client, const connect_opt *opt);
client_err_t disconnect_mqtt_server(remote_client_t *client);
client_err_t start_mqtt_main_loop(remote_client_t *client);
void stop_mqtt_main_loop(remote_client_t *client);
client_err_t mqtt_data_upload(remote_client_t *client, const void *payload, int len);


#endif