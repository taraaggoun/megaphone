#include "network/server/udp_server.h"

#include <string.h>

#include "network/server/server.h"
#include "network/server/network.h"
#include "network/server/request_manager.h"

#include "system/logger.h"

static Server m_server;


/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

void *udp_server_loop(__attribute__((unused)) void *args)
{
	ClientRQ clientrq;
	ssize_t nbytes;

	memset(&clientrq, 0, sizeof(clientrq));
	while (1) {
		nbytes = recv_datagrams(m_server.sfd, &clientrq);
		if (nbytes < 0)
			break;

		if (handle_upload_packet(&clientrq, (size_t) nbytes))
			break;
	}

	return NULL;
}

uint8_t udp_server_init(in_port_t port)
{
	if (create_udp_server(&m_server, port))
		return 1;

	logsuccess("UDP Server Initialazed");
	return 0;
}

/* -------------------------------------------------------------------------- */
