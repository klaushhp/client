#ifndef _OUT_PACKET_H_
#define _OUT_PACKET_H_

#include "common.h"

client_err_t packet_alloc(tcp_packet_t* packet);
client_err_t packet_queue(remote_client_t* client, tcp_packet_t* packet);
client_err_t packet_write(remote_client_t* client);
client_err_t packet_read(remote_client_t* client);
void packet_init(tcp_packet_t* packet);
void packet_cleanup_all(tcp_client* client);

#endif