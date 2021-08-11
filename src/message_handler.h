#ifndef _MESSAGE_HADLER_H_
#define _MESSAGE_HADLER_H_

#include "common.h"

void* tcp_message_handle_loop(void *obj);
void message_cleanup_all(tcp_client *client);

typedef enum
{
    CMD_ID_QUERY_PARAM = 0x80,
    CMD_ID_SET_PARAM = 0x81,
    CMD_ID_VEHICLE_CONTROL = 0x82,
    CMD_ID_REQUEST_CAN_MSG = 0x93,
} cmd_id_t;

#endif