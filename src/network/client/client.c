#include "network/client/client.h"

#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include "network/network_macros.h"

static char m_hostname[HOSTNAME_STRLEN];


/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

void set_hostname(const char *hostname)
{
	strncpy(m_hostname, hostname, HOSTNAME_STRLEN);
}

Client client_new(const char *port, int domain, int service)
{
	Client client;

	client.port = port;
	client.domain = domain;
	client.service = service;

	return client;
}

uint8_t connect_client(Client *client) {

	/* Setting up addrinfo structure */
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = client->domain;
	hints.ai_socktype = client->service;
	hints.ai_flags = AI_V4MAPPED | AI_ALL;
	/* ----------------------------- */

	/* Getting addresses */
	struct addrinfo *res = NULL;
	int info_res;

	info_res = getaddrinfo(m_hostname, client->port, &hints, &res);
	if (info_res != 0 || res == NULL) {
		perror("getaddrinfo");
		return 1;
	}

	struct addrinfo *p = res;
	int sfd;

	while (p != NULL) {
		int domain = p->ai_family;
		int type = p->ai_socktype;
		int protocol = p->ai_protocol;

		if ((sfd = socket(domain, type, protocol)) > 0) {
			if (client->service == SOCK_DGRAM)
				break;

			if (connect(sfd, p->ai_addr, p->ai_addrlen) == 0)
				break;

			close(sfd);
		}

		p = p->ai_next;
	}

	if (p == NULL) {
		/* connection failed */
		perror("connection failed");
		freeaddrinfo(res);
		return 1;
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"

	client->sfd = sfd;
	client->addr = *((struct sockaddr_in6 *) p->ai_addr);

#pragma GCC diagnostic pop

	freeaddrinfo(res);
	/* ----------------- */

	return 0;
}

/* -------------------------------------------------------------------------- */
