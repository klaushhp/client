#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h> 
#include <sys/errno.h>
#include <unistd.h>

#include "tcp_socket.h"


client_err_t set_socket_nonblock(int* sock)
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

client_err_t local_socketpair(int* pair_r, int* pair_w)
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

client_err_t connect_server(tcp_client* client)
{
    struct  sockaddr_in serv_addr;

    if( client->sockfd != INVALID_SOCKET ) {
        close(client->sockfd);
    }

    client->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( client->sockfd == INVALID_SOCKET )
    {
        return CLIENT_ERR_ERRNO;
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(client->address);
    serv_addr.sin_port = htons(client->port);

    int ret = connect(client->sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if( ret )
    {
        printf("connect failed [%d]\n", ret);
        printf("%s\n",strerror(errno));
        return CLIENT_ERR_ERRNO;
    }
    
    return CLIENT_ERR_SUCCESS;
}

static void disconnect_server(tcp_client* client)
{
    close(client->sockfd);
    client->sockfd = INVALID_SOCKET;

    return ;
}

static int tcp_client_loop(tcp_client* client)
{
    fd_set readfds, writefds;
    int fdcount;
    int maxfd = 0;
    uint8_t pairbuf;
    client_err_t ret = CLIENT_ERR_SUCCESS;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    if( client->sockfd != INVALID_SOCKET ) {
        maxfd = client->sockfd;
        FD_SET(client->sockfd, &readfds);
        if(client->msg)
        {
            FD_SET(client->sockfd, &writefds);
        }    
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
            printf("sockfd can read-------\n");
#if 0
            char buf[128];
            int ret;
            ret = recv(client->sockfd, buf, sizeof(buf), 0);
            if(ret == 0)
            {
                printf("lost connect------\n");
                return CLIENT_ERR_CONN_LOST;
            }

            printf("receive: %s\n", buf);
#endif
            return CLIENT_ERR_SUCCESS;
        }

        if( client->sockpair_r != INVALID_SOCKET  && FD_ISSET(client->sockpair_r, &readfds) ) {
            recv(client->sockpair_r, &pairbuf, 1, 0);
            
            switch ((client_internal_cmd)pairbuf) 
            {
            case internal_cmd_connect:
                printf("internal_cmd_connect\n");
                ret = connect_server(client);
                if( ret == CLIENT_ERR_SUCCESS )
                {
                    client_set_state(client, tcp_cs_connected);
                    printf("publish tcp connect success\n");
                    /*TODO: publish tcp connect success*/
                } 
                return ret;   

            case internal_cmd_disconnect:
                printf("internal_cmd_disconnect\n");
                disconnect_server(client);
                client_set_state(client, tcp_cs_disconnected);
                /*TODO: publish tcp disconnect success*/
               
                return CLIENT_ERR_SUCCESS;

            case internal_cmd_write_trigger:
                
                break;

            case internal_cmd_force_quit:
                
                break;
            
            default:
                break;
            }
        }

        if( client->sockfd != INVALID_SOCKET && FD_ISSET(client->sockfd, &writefds) ) {           
            printf("sockfd can write------\n");

        }

    }
    else
    {
        return CLIENT_ERR_ERRNO;
    }

    return CLIENT_ERR_SUCCESS;
}


void* tcp_client_main_loop(void* obj)
{
    int rc;
    int run = 1;
    int state;
    tcp_client* client = (tcp_client*)obj;

    while( run ) {
        
        do {
            rc = tcp_client_loop(client);

        } while(run == 1 && rc == CLIENT_ERR_SUCCESS);
        
        switch (rc) 
        {
        case CLIENT_ERR_NOMEN:
            /*TODO: err handle*/
            printf("Error: [%d]\n", rc);
            return (void*)rc;

        case CLIENT_ERR_ERRNO: 
        case CLIENT_ERR_CONN_LOST:             
            break;
        
        default:
            printf("Unknow error need to handle!\n");
            break;
        }

        do {
            state = client_get_state(client);
            if(state == tcp_cs_disconnected) {
                run = 0;
            } else {
                printf("do reconnect\n");
                sleep(1);
                rc = connect_server(client);
            }

            /*TODO: use select to delay, can interuppt the delay*/
        } while(run && rc != CLIENT_ERR_SUCCESS);

    }
}
