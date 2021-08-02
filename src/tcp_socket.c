#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h> 
#include <sys/errno.h>
#include <unistd.h>

#include "tcp_socket.h"
#include "tcp_packet.h"

static client_err_t tcp_loop_read(net_client* client);
static client_err_t tcp_loop_write(net_client* client);

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

client_err_t try_connect(int* sock, const char* host, uint16_t port)
{
    struct  sockaddr_in serv_addr;

    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if( *sock == INVALID_SOCKET )
    {
        return CLIENT_ERR_ERRNO;
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(host);
    serv_addr.sin_port = htons(port);

    int ret = connect(*sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if( ret )
    {
        printf("connect failed [%d] %s:%d\n", ret, host, port);
        printf("%s\n", strerror(errno));
        close(*sock);
        return CLIENT_ERR_ERRNO;
    }
    
    return CLIENT_ERR_SUCCESS;
}

client_err_t connect_step_2(tcp_client* client)
{
#ifdef ENABLE_TLS


#endif

    return CLIENT_ERR_SUCCESS;
}

client_err_t connect_server(tcp_client* client)
{
    client_err_t ret = CLIENT_ERR_SUCCESS;

    if( client->sockfd != INVALID_SOCKET ) {
        close(client->sockfd);
    }

    ret = try_connect(&client->sockfd, client->address, client->port);
    if( ret > 0 )
        return ret;

    ret = connect_step_2(client);        

    return ret;
}


static void disconnect_server(tcp_client* client)
{
    close(client->sockfd);
    client->sockfd = INVALID_SOCKET;

    /*TODO: free data*/
    return ;
}

static int tcp_client_loop(net_client* client)
{
    fd_set readfds, writefds;
    int fdcount;
    int maxfd = 0;
    uint8_t pairbuf;
    tcp_client* clt = NULL;
    client_err_t ret = CLIENT_ERR_SUCCESS;

    if( !client || !client->tcp_clt ) {
        return CLIENT_ERR_INVALID_PARAM;
    }
    clt = client->tcp_clt;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    if( clt->sockfd != INVALID_SOCKET ) {
        maxfd = clt->sockfd;
        FD_SET(clt->sockfd, &readfds);

        if( clt->out_packet )
        {
            FD_SET(clt->sockfd, &writefds);
        }    
    } else {
        return CLIENT_ERR_NO_CONN;
    }

    if( clt->sockpair_r != INVALID_SOCKET ) {
        FD_SET(clt->sockpair_r, &readfds);

        if( clt->sockpair_r > maxfd ) {
            maxfd = clt->sockpair_r;
        }
    }
    
    fdcount = select(maxfd+1, &readfds, &writefds, NULL, NULL);
    if( fdcount != -1 )
    {
        if( clt->sockfd != INVALID_SOCKET ) { 
            if( FD_ISSET(clt->sockfd, &readfds) ) {
                printf("sockfd can read-------\n");
                ret = tcp_loop_read(client);

                if(ret || clt->sockfd == INVALID_SOCKET)
                {
                    return ret;
                }
            }

            if( clt->sockpair_r != INVALID_SOCKET  && FD_ISSET(clt->sockpair_r, &readfds) ) {
                recv(clt->sockpair_r, &pairbuf, 1, 0);
                
                switch ((client_internal_cmd)pairbuf) 
                {
                case internal_cmd_disconnect:
                    printf("internal_cmd_disconnect\n");
                    disconnect_server(clt);
                    client_set_state(clt, tcp_cs_disconnected);
                    /*TODO: publish tcp disconnect success*/
                
                    return CLIENT_ERR_SUCCESS;

                case internal_cmd_write_trigger:
                    if( clt->sockfd != INVALID_SOCKET ) {
                        FD_SET(clt->sockfd, &writefds);
                    }    
                    break;

                default:
                    break;
                }
            }

            if( clt->sockfd != INVALID_SOCKET && FD_ISSET(clt->sockfd, &writefds) ) {           
                printf("sockfd can write------\n");
                ret = tcp_loop_write(client);
                if( ret || clt->sockfd == INVALID_SOCKET )
                {
                    return ret;
                }
            }
        }
    }
    else
    {
        ret = CLIENT_ERR_ERRNO;
    }

    return ret;
}


void* tcp_client_main_loop(void* obj)
{
    int rc;
    int run = 1;
    int state;
    net_client* client = (net_client*)obj;

    while( run ) {
        
        do {
            pthread_testcancel();
            rc = tcp_client_loop(client);
        } while(run == 1 && rc == CLIENT_ERR_SUCCESS);
        
        switch (rc) 
        {
        case CLIENT_ERR_NOMEN:
        case CLIENT_ERR_INVALID_PARAM:
            printf("Fatal err!!: [%d]--------------------------------\n", rc);
            return (void*)rc;

        case CLIENT_ERR_ERRNO:
        case CLIENT_ERR_NO_CONN:
        case CLIENT_ERR_CONN_LOST:           
            break;
        
        default:
            printf("Unknow error need to handle!\n");
            break;
        }

        do {
            pthread_testcancel();

            state = client_get_state(client->tcp_clt);
            if(state == tcp_cs_disconnected) {
                run = 0;
            } else {
                printf("do reconnect\n");
                sleep(1);
                rc = connect_server(client->tcp_clt);
            }

            /*TODO: use select to delay, can interuppt the delay*/
        } while(run && rc != CLIENT_ERR_SUCCESS);

    }
}

static client_err_t tcp_loop_read(net_client* client)
{
    return packet_read(client);
}

static client_err_t tcp_loop_write(net_client* client)
{
    return packet_write(client->tcp_clt);
}
