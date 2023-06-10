#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H


/* -------------------------------- INCLUDE --------------------------------- */

#include "network/network_macros.h"

/* -------------------------------- FUNCTIONS ------------------------------- */

uint8_t tcp_client_init(const char *port);
void *tcp_client_loop(__attribute__((unused)) void *arg);

/* -------------------------------------------------------------------------- */

#endif /* TCP_CLIENT_H */
