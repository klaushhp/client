#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <utlist.h>

#define INVALID_SOCKET -1
#define INVALID_HANDLE -1
#define INVALID_THREAD -1

typedef enum
{
    PROTOCAL_TCP = 0,
    PROTOCAL_MQTT = 1,
} client_protocal_type;

typedef enum
{
    tcp_cs_new = 0,
    tcp_cs_connected = 1,
    tcp_cs_disconnected = 2,
    tcp_cs_connect_pending = 3,
} tcp_client_state;

#define MSG_START_ID        (0x23)
#define MSG_START_ID_LEN    (2)
#define MSG_CMD_ID_LEN      (1)
#define MSG_ACK_FLAG_LEN    (1)
#define MSG_DATE_LEN        (3)
#define MSG_TIME_LEN        (3)
#define MSG_DATA_LEN        (2)
#define MSG_CHECK_SUM_LEN   (1)
#define MSG_BEFORE_DATA_LEN (2+1+1+3+3+2)

struct tcp_packet_msg
{
    uint8_t cmd_id;
    uint8_t ack_flag;
    uint8_t date[MSG_DATE_LEN];
    uint8_t time[MSG_TIME_LEN];
    void* payload;
    int payload_len;
    uint8_t check_sum;
    struct tcp_packet_msg* prev;
    struct tcp_packet_msg* next;
};
typedef struct tcp_packet_msg tcp_packet_msg_t;


/*tcp packet has two directions      */
/*For the packet to send out, the payload means the whole message, payload_len is the whole message len */
/*For the packet received, the payload is the real payload part,  payload_len is only the payload part len  */
struct tcp_packet
{
    void* payload;
    int payload_len;
    struct tcp_packet* next;
    int pos;
    int to_process;
    uint8_t head_buf[MSG_BEFORE_DATA_LEN]; 
} ;
typedef struct tcp_packet tcp_packet_t;

typedef struct
{
    int sockfd;
    int sockpair_r, sockpair_w;
    char* host;
    uint16_t port;
    tcp_client_state state;
    pthread_t client_thread_id;
    pthread_t msg_thread_id;
    tcp_packet_t* out_packet;
    tcp_packet_t* out_packet_last;
    tcp_packet_t in_packet;
    tcp_packet_msg_t* in_msg;
    pthread_mutex_t state_mutex;    
    pthread_mutex_t out_packet_mutex;
    pthread_mutex_t in_msg_mutex;
    sem_t in_msg_sem;
} tcp_client;

typedef enum
{
    internal_cmd_break_block = 0,
    internal_cmd_disconnect = 1,
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
    CLIENT_ERR_INVALID_PROTOCAL =7,
} client_err_t;

void send_internal_signal(int sock, client_internal_cmd cmd);
int client_set_state(tcp_client* client, tcp_client_state state);
tcp_client_state client_get_state(tcp_client* client);

typedef int client_t;

struct remote_client
{
    client_t handle;
    client_protocal_type type; 
    //bool login_status;
    //pthread_mutex_t login_mutex;
    void* clt;
    struct remote_client* prev;
    struct remote_client* next;
};
typedef struct remote_client remote_client_t;



#endif 