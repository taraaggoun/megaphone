#ifndef SERVER_H
#define SERVER_H

/* -------------------------------- INCLUDE --------------------------------- */

#include "network/network_macros.h"

/* -------------------------------- STRUCTURE ------------------------------- */

typedef struct
{
	in_port_t port;
	sa_family_t domain;

	int service;

	int sfd;
	SA_IN6 addr;
} Server;

/* -------------------------------- FUNCTIONS ------------------------------- */

uint8_t create_tcp_server(Server *server, in_port_t port);

uint8_t create_udp_server(Server *server, in_port_t port);

/* -------------------------------------------------------------------------- */

#endif /* SERVER_H */
