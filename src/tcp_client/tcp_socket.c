#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h> 
#include <sys/errno.h>
#include <netdb.h>
#include <unistd.h>

#include "tcp_socket.h"
#include "tcp_packet.h"

static client_err_t tcp_loop_read(tcp_client* client);
static client_err_t tcp_loop_write(tcp_client* client);

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

client_err_t try_connect(int* sock, const char* host, uint16_t port)
{
    struct addrinfo hints;
    struct addrinfo *ainfo, *rp;
    struct sockaddr_in serv_addr;
    int s;
    int ret;
    
    *sock = INVALID_SOCKET;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    s = getaddrinfo(host, NULL, &hints, &ainfo);
    if(s) {
        printf("Fatal: can't transfer host info\n");
        return CLIENT_ERR_ERRNO;
    }
    
    for(rp = ainfo; rp != NULL; rp = rp->ai_next) {
        *sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        printf("Get sockfd [%d]\n", *sock);
        if(*sock == INVALID_SOCKET) {
            continue;
        }

        if(rp->ai_family == AF_INET){
			((struct sockaddr_in *)rp->ai_addr)->sin_port = htons(port);
		}else if(rp->ai_family == AF_INET6){
			((struct sockaddr_in6 *)rp->ai_addr)->sin6_port = htons(port);
		}else{
			close(*sock);
			*sock = INVALID_SOCKET;
			continue;
		}

        ret = connect(*sock, rp->ai_addr, rp->ai_addrlen);
        if( ret == 0 )
        {
            break;
        }

        close(*sock);
        *sock = INVALID_SOCKET;
    }
    
    freeaddrinfo(ainfo);
    if( !rp ) {
        return CLIENT_ERR_ERRNO;
    }

    return ret;
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
        net_socket_close(client);
    }

    ret = try_connect(&client->sockfd, client->host, client->port);
    if( ret > 0 ) {
        return ret;
    }    

    ret = connect_step_2(client);
    if(ret == CLIENT_ERR_SUCCESS) 
    {
        client_set_state(client, tcp_cs_connected);
    } else {
        client_set_state(client, tcp_cs_connect_pending);
    }       

    return ret;
}

void net_socket_close(tcp_client* client)
{

#ifdef ENABLE_TLS


#endif
    if( client->sockfd != INVALID_SOCKET ) {
        close(client->sockfd);
        client->sockfd = INVALID_SOCKET;
    }    

    return ;
}

static int tcp_client_loop(tcp_client* client)
{
    fd_set readfds, writefds;
    int fdcount;
    int maxfd = 0;
    uint8_t pairbuf;
    client_err_t ret = CLIENT_ERR_SUCCESS;

    if( !client ) {
        return CLIENT_ERR_INVALID_PARAM;
    }

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    if( client->sockfd != INVALID_SOCKET ) {
        maxfd = client->sockfd;
        FD_SET(client->sockfd, &readfds);

        if( client->out_packet )
        {
            FD_SET(client->sockfd, &writefds);
        }    
    } else {
        return CLIENT_ERR_NO_CONN;
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
        if( client->sockfd != INVALID_SOCKET ) { 
            if( FD_ISSET(client->sockfd, &readfds) ) {
                printf("sockfd can read-------\n");
                ret = tcp_loop_read(client);

                if(ret || client->sockfd == INVALID_SOCKET)
                {
                    return ret;
                }
            }

            if( client->sockpair_r != INVALID_SOCKET  && FD_ISSET(client->sockpair_r, &readfds) ) {
                recv(client->sockpair_r, &pairbuf, 1, 0);
                
                switch ((client_internal_cmd)pairbuf) 
                {
                case internal_cmd_break_block:
                    /*Do nothing, just break select*/
                    break;

                case internal_cmd_disconnect:
                    printf("internal_cmd_disconnect\n");
                    net_socket_close(client);
                    client_set_state(client, tcp_cs_disconnected);
                    packet_cleanup_all(client);
                    /*TODO: publish tcp disconnect success*/
                
                    return CLIENT_ERR_SUCCESS;

                case internal_cmd_write_trigger:
                    if( client->sockfd != INVALID_SOCKET ) {
                        FD_SET(client->sockfd, &writefds);
                    }    
                    break;

                default:
                    break;
                }
            }

            if( client->sockfd != INVALID_SOCKET && FD_ISSET(client->sockfd, &writefds) ) {           
                printf("sockfd can write------\n");
                ret = tcp_loop_write(client);
                if( ret || client->sockfd == INVALID_SOCKET )
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
    tcp_client* client = (tcp_client*)obj;

    while( run ) {
        
        do {
            pthread_testcancel();
            rc = tcp_client_loop(client);
        } while(run == 1 && rc == CLIENT_ERR_SUCCESS);
        
        switch (rc) 
        {
        case CLIENT_ERR_NOMEN:
        case CLIENT_ERR_INVALID_PARAM:
            printf("Fatal err!!: [%d], thread exit!!! \n", rc);
            return (void*)(-1);

        case CLIENT_ERR_ERRNO:
        case CLIENT_ERR_NO_CONN:
        case CLIENT_ERR_CONN_LOST:
            printf("Warning: [%d]\n", rc);           
            break;
        
        default:
            printf("Unknow error need to handle!\n");
            break;
        }

        do {
            pthread_testcancel();

            state = client_get_state(client);
            if(state == tcp_cs_disconnected) {
                printf("Info status disconnected, thread exit\n");
                run = 0;
            } else {
                printf("do reconnect\n");
                sleep(1);
                rc = connect_server(client);
            }

            /*TODO: use select to delay, can interuppt the delay*/
        } while(run && rc != CLIENT_ERR_SUCCESS);

    }

    return (void*)0;
}

static client_err_t tcp_loop_read(tcp_client* client)
{
    return packet_read(client);
}

static client_err_t tcp_loop_write(tcp_client* client)
{
    return packet_write(client);
}
