#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include "common.h"
#include <pthread.h>

#define INVALID_SOCKET -1

typedef enum
{
    tcp_cs_new = 0,
    tcp_cs_connected = 1,
    tcp_cs_disconnected = 2,
} tcp_client_state;

typedef enum
{
    internal_cmd_connect = 0,
    internal_cmd_disconnect = 1,
    internal_cmd_write_trigger = 2,
    internal_cmd_force_quit = 3,
} client_internal_cmd;

struct out_packet
{
    void* payload;
    int payload_len;
    struct out_packet* next;
} ;
typedef struct out_packet out_packet_t; 

typedef struct
{
    char client_name[16];
    int sockfd;
    int sockpair_r, sockpair_w;
    char address[16];
    uint16_t port;
    tcp_client_state state;
    bool login_status;
    pthread_t thread_id;
    out_packet_t* msg;
    pthread_mutex_t state_mutex;
    pthread_mutex_t packet_mutex;
} tcp_client;

client_err_t create_tcp_client(char* name);
void destory_tcp_client();
client_err_t connect_to_tcp_server(char* ip, uint16_t port);


#endif