#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h> 
#include <sys/errno.h>
#include <unistd.h> 

#include "tcp_client.h"

static tcp_client* client = NULL;

static client_err_t set_socket_nonblock(int* sock)
{
    int opt;

	opt = fcntl(*sock, F_GETFL, 0);
	if(opt == -1) {
		close(*sock);
		*sock = INVALID_SOCKET;
		return CLIENT_ERR_ERRNO;
	}

	if(fcntl(*sock, F_SETFL, opt | O_NONBLOCK) == -1) {
		close(*sock);
		*sock = INVALID_SOCKET;
		return CLIENT_ERR_ERRNO;
	}

    return CLIENT_ERR_SUCCESS;
}

static client_err_t local_socketpair(int* pair_r, int* pair_w)
{
    int sv[2];

	*pair_r = INVALID_SOCKET;
	*pair_w = INVALID_SOCKET;

	if(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1){
		return CLIENT_ERR_ERRNO;
	}
	if(set_socket_nonblock(&sv[0])){
		close(sv[1]);
        sv[1] = INVALID_SOCKET;
		return CLIENT_ERR_ERRNO;
	}
	if(set_socket_nonblock(&sv[1])){
		close(sv[0]);
        sv[0] = INVALID_SOCKET;
		return CLIENT_ERR_ERRNO;
	}
	*pair_r = sv[0];
	*pair_w = sv[1];

	return CLIENT_ERR_SUCCESS;
}

static int client_set_state(tcp_client_state state)
{
	pthread_mutex_lock(&client->state_mutex);

	client->state = state;
	
	pthread_mutex_unlock(&client->state_mutex);

	return CLIENT_ERR_SUCCESS;
}

static tcp_client_state client_get_state()
{
	tcp_client_state state;

	pthread_mutex_lock(&client->state_mutex);

	state = client->state;

	pthread_mutex_unlock(&client->state_mutex);

	return state;
}

static client_err_t connect_server()
{
    struct  sockaddr_in serv_addr;

    if( client->sockfd == INVALID_SOCKET ) {
        client->sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if( client->sockfd == INVALID_SOCKET )
        {
            return CLIENT_ERR_ERRNO;
        }
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(client->address);
    serv_addr.sin_port = htons(client->port);

    if( connect(client->sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) )
    {
        return CLIENT_ERR_ERRNO;
    }
    
    return CLIENT_ERR_SUCCESS;
}

static void disconnect_server()
{
    close(client->sockfd);
    client->sockfd = INVALID_SOCKET;

    return ;
}

static int tcp_client_loop()
{
    fd_set readfds, writefds;
    int fdcount;
    int maxfd = 0;
    uint8_t pairbuf;
    client_err_t ret = CLIENT_ERR_SUCCESS;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    if( client->sockfd == INVALID_SOCKET ) {
        maxfd = client->sockfd;
        FD_SET(client->sockfd, &readfds);
        FD_SET(client->sockfd, &writefds);
    }

    if( client->sockpair_r != INVALID_SOCKET ) {
        FD_SET(client->sockpair_r, &readfds);
        if( client->sockpair_r > maxfd ) {
            maxfd = client->sockpair_r;
        }
    }
    
    fdcount = select(maxfd+1, &readfds, &writefds, NULL, NULL);
    if( fdcount != -1 )
    {
        if( client->sockfd != INVALID_SOCKET && FD_ISSET(client->sockfd, &readfds) ) {
            

        }

        if( client->sockpair_r != INVALID_SOCKET  && FD_ISSET(client->sockpair_r, &readfds) ) {
            recv(client->sockpair_r, &pairbuf, 1, 0);
            
            switch ((client_internal_cmd)pairbuf) 
            {
            case internal_cmd_connect:
                printf("internal_cmd_connect\n");
                ret = connect_server();
                if( ret == CLIENT_ERR_SUCCESS )
                {
                    client_set_state(tcp_cs_connected);
                    printf("publish tcp connect success\n");
                    /*TODO: publish tcp connect success*/
                } 
                return ret;   
                break;

            case internal_cmd_disconnect:
                printf("internal_cmd_disconnect\n");
                disconnect_server();
                client_set_state(tcp_cs_disconnected);
                /*TODO: publish tcp disconnect success*/
               
                return CLIENT_ERR_SUCCESS;
                break;

            case internal_cmd_write_trigger:
                
                break;

            case internal_cmd_force_quit:
                
                break;
            
            default:
                break;
            }
        }

        if( client->sockfd != INVALID_SOCKET && FD_ISSET(client->sockfd, &writefds) ) {
            

        }


    }
    else
    {
        return CLIENT_ERR_ERRNO;
    }

    return CLIENT_ERR_SUCCESS;
}


static void* tcp_client_main_loop(void* obj)
{
    int rc = 0;
    int run = 1;

    while( run ) {
        rc = tcp_client_loop();
        
        switch (rc) 
        {
        case CLIENT_ERR_CONN_LOST:
            /* code */
            break;
        
        case CLIENT_ERR_FORCE_QUIT:
            run = 0;
            break;

        default:
            break;
        }
    }
}



static client_err_t tcp_client_init(char* name)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    strncpy(client->client_name, name, sizeof(client->client_name));
    client->sockfd = INVALID_SOCKET;
    memset(client->address, 0, sizeof(client->address));
    client->port = 0;
    client->state = tcp_cs_new;
    client->login_status = false;
    client->msg = NULL;

    pthread_mutex_init(&client->state_mutex, NULL);
    pthread_mutex_init(&client->packet_mutex, NULL);

    if( local_socketpair(&client->sockpair_r, &client->sockpair_w) )
    {
        ret = CLIENT_ERR_ERRNO;
    }

    if( ret ==  CLIENT_ERR_SUCCESS )
    {
        if( pthread_create(&client->thread_id, NULL, tcp_client_main_loop, NULL) == 0 )
        {
            pthread_detach(client->thread_id);
        }
        else
        {
            ret = CLIENT_ERR_ERRNO;
        }     
    }

    return ret;
}

client_err_t create_tcp_client(char* name)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    if( client != NULL )
    {
        return CLIENT_ERR_SINGLETON;
    }

    client = (tcp_client *)calloc(1, sizeof(tcp_client));
    if( client == NULL )
    {
        return CLIENT_ERR_NOMEN;
    }

    ret = tcp_client_init(name) ;
    if( ret )
    {
        destory_tcp_client();
    }

    return ret;
}

void destory_tcp_client()
{



}

client_err_t connect_to_tcp_server(char* ip, uint16_t port)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;
    client_internal_cmd cmd = internal_cmd_connect;

    if( (client != NULL) && (client_get_state() != tcp_cs_connected) )
    {
        strncpy(client->address, ip, sizeof(client->address));
        client->port = port;

        send(client->sockpair_w, &cmd, 1, 0);
    }
    else
    {
        ret = CLIENT_ERR_INVALID_OPERATION;
    }

    return ret;
}