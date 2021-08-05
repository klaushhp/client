#ifndef _OUT_PACKET_H_
#define _OUT_PACKET_H_

#include "common.h"

client_err_t packet_alloc(tcp_packet_t* packet);
client_err_t packet_queue(tcp_client* client, tcp_packet_t* packet);
client_err_t packet_write(tcp_client* client);
client_err_t packet_read(tcp_client* client);
void packet_cleanup_all(tcp_client* client);

#endif