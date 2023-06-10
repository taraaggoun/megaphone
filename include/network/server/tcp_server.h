#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "network/network_macros.h"

/* -------------------------------- FUNCTIONS ------------------------------- */

uint8_t tcp_server_init(in_port_t port);

void *tcp_server_loop(__attribute__((unused)) void *args);

/* -------------------------------------------------------------------------- */

#endif /* TCP_SERVER_H */
