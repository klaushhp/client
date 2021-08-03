#include <sys/socket.h>
#include "common.h"


int client_set_state(tcp_client* client, tcp_client_state state)
{
	pthread_mutex_lock(&client->state_mutex);

	client->state = state;
	
	pthread_mutex_unlock(&client->state_mutex);

	return CLIENT_ERR_SUCCESS;
}

tcp_client_state client_get_state(tcp_client* client)
{
	tcp_client_state state;

	pthread_mutex_lock(&client->state_mutex);

	state = client->state;

	pthread_mutex_unlock(&client->state_mutex);

	return state;
}

void send_internal_signal(int sock, client_internal_cmd cmd)
{
    send(sock, &cmd, 1, 0);
}