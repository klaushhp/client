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
#include <uthash.h>
#include <mosquitto.h>


/*************************************FOR TCP PROTOCAL****************************************/
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

/*tcp packet has two directions      */
/*For the packet to send out, the payload means the whole message, payload_len is the whole message len */
/*For the packet received, the payload is the real payload part,  payload_len is only the payload part len  */
struct tcp_packet
{
    uint8_t* payload;
    int payload_len;
    struct tcp_packet* next;
    int pos;
    int to_process;
    uint8_t head_buf[12];
    int sockpair_r, sockpair_w; 
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
    tcp_packet_t* out_packet;
    tcp_packet_t* out_packet_last;
    tcp_packet_t* current_out_packet;
    tcp_packet_t in_packet;
    pthread_mutex_t state_mutex;    
    pthread_mutex_t out_packet_mutex;
    pthread_mutex_t current_out_packet_mutex;
} tcp_client;

typedef enum
{
    internal_cmd_break_block = 0,
    internal_cmd_disconnect = 1,
    internal_cmd_write_trigger = 2,
} client_internal_cmd;

/**************************************FOR MQTT PROTOCAL****************************************/
#if 0
typedef struct 
{
    int mid;
    int fd;
    UT_hash_handle hh;
} out_msg_hash_table;
#endif
typedef struct 
{
    struct mosquitto *mosq;
    //out_msg_hash_table *out_msg_table; 
    //pthread_mutex_t out_msg_mutex;
    pthread_t client_thread_id;
} mqtt_client;


typedef struct 
{
    int *mid;
    char *topic;
    void *payload;
    int payload_len;
    int qos;
    bool retain;
} mqtt_msg;



/********************************************Common*********************************************/
#define INVALID_SOCKET -1
#define INVALID_HANDLE -1
#define INVALID_THREAD -1

#define MSG_START_ID        (0x23)
#define MSG_START_ID_LEN    (2)
#define MSG_CMD_ID_LEN      (1)
#define MSG_ACK_FLAG_LEN    (1)
#define MSG_DATE_LEN        (3)
#define MSG_TIME_LEN        (3)
#define MSG_DATA_LEN        (2)
#define MSG_CHECK_SUM_LEN   (1)
#define MSG_BEFORE_DATA_LEN (2+1+1+3+3+2)

struct pending_msg
{
    uint8_t cmd_id;
    uint8_t ack_flag;
    uint8_t date[MSG_DATE_LEN];
    uint8_t time[MSG_TIME_LEN];
    void* payload;
    int payload_len;
    uint8_t check_sum;
    struct pending_msg* prev;
    struct pending_msg* next;
};
typedef struct pending_msg pending_msg_t;

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

typedef uint32_t client_t;

typedef struct 
{
    client_protocal_type type;
    char *host;
    uint16_t port;
} connect_opt;

struct remote_client
{
    client_protocal_type type; 
    client_t handle;
    pthread_t msg_thread_id;
    pending_msg_t *in_msg;
    pthread_mutex_t in_msg_mutex;
    sem_t in_msg_sem;
    void *clt;

    void* (* create_client)();
    void (* destroy_client)(struct remote_client* client);
    client_err_t (* connect_server)(struct remote_client* client, const connect_opt* opt);
    client_err_t (* disconnect_server)(struct remote_client* client);
    client_err_t (* start_main_loop)(struct remote_client* client);
    void (* stop_main_loop)(struct remote_client* client);
    client_err_t (* data_upload)(struct remote_client* client, const void* payload, int len);

    struct remote_client *prev;
    struct remote_client *next;
};
typedef struct remote_client remote_client_t;

client_err_t set_socket_nonblock(int *sock);
client_err_t local_socketpair(int *pair_r, int *pair_w, bool block);



#endif 