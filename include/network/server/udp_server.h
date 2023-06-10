#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "network/network_macros.h"

/* -------------------------------- FUNCTIONS ------------------------------- */

uint8_t udp_server_init(in_port_t port);

void *udp_server_loop(__attribute__((unused)) void *args);

/* -------------------------------------------------------------------------- */

#endif /* UDP_SERVER_H */
