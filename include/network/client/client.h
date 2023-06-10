#ifndef CLIENT_H
#define CLIENT_H


/* -------------------------------- INCLUDES -------------------------------- */

#include <sys/socket.h>
#include <netinet/in.h>

/* -------------------------------- STRUCTURE ------------------------------- */

typedef struct
{
	const char *port;

	int domain;
	int service;

	int sfd;
	struct sockaddr_in6 addr;
} Client;

/* -------------------------------- FUNCTIONS ------------------------------- */

void set_hostname(const char *hostname);
Client client_new(const char *port, int domain, int service);
uint8_t connect_client(Client *client);

/* -------------------------------------------------------------------------- */

#endif /* CLIENT_H */
