#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#ifdef USE_TCP

#define INVALID_SOCKET -1

typedef enum
{
    tcp_cs_new = 0,
    tcp_cs_connected = 1,
    tcp_cs_disconnected = 2,
    tcp_cs_connect_pending = 3,
} tcp_client_state;

struct tcp_packet
{
    void* payload;
    int payload_len;
    struct tcp_packet* next;
} ;
typedef struct tcp_packet tcp_packet_t;

typedef struct
{
    char client_name[16];
    int sockfd;
    int sockpair_r, sockpair_w;
    char address[16];
    uint16_t port;
    tcp_client_state state;
    pthread_t thread_id;
    tcp_packet_t* out_packet;
    tcp_packet_t* out_packet_last;
    pthread_mutex_t state_mutex;    
    pthread_mutex_t packet_mutex;
} tcp_client;

typedef enum
{
    internal_cmd_disconnect = 0,
    internal_cmd_write_trigger = 2,
} client_internal_cmd;

typedef enum 
{
    CLIENT_ERR_SUCCESS = 0,
    CLIENT_ERR_NOMEN = 1,
    CLIENT_ERR_INVALID_PARAM = 2,
    CLIENT_ERR_INVALID_OPERATION = 3,
    CLIENT_ERR_ERRNO = 4,
    CLIENT_ERR_NO_CONN = 5,
    CLIENT_ERR_CONN_LOST = 6,
} client_err_t;

#elif defined(USE_MQTT)




#endif

typedef struct
{
#ifdef USE_TCP
tcp_client* tcp_clt;
#elif defined(USE_MQTT)

#endif
bool login_status;
pthread_mutex_t login_mutex;

} net_client;


#endif 