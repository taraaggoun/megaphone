#include "network/client/udp_client.h"

#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "network/request.h"

#include "network/client/data.h"
#include "network/client/network.h"
#include "network/client/request_manager.h"

#include "system/logger.h"

static struct pollfd fds;


/* ---------------------------- PRIVATE FUNCTIONS --------------------------- */

static int8_t handle_ready_fds(void)
{
	ServerRQ serverrq;
	ssize_t nbytes;
	int8_t err;

	memset(&serverrq, 0, sizeof(serverrq));
	if (!(fds.revents & POLLIN))
		return 1;

	while (1) {
		nbytes = recv_datagrams(fds.fd, &serverrq);
		if (nbytes < 0 && SBLOCK)
			return 1;

		if (nbytes < 0)
			return -1;

		err = handle_download_packet(&serverrq, (size_t) nbytes);
		if (err != 1) {
			reset_transfer_socket();
			return err;
		}
	}
}

static int8_t handle_download(void)
{
	int timeout = 1000 * FT_TIMEOUT_SEC;

	if (check_transfer_timeout())
		return 0;

	switch (poll(&fds, 1, timeout)) {
	case -1:
		perror("poll");
		return -1;

	case 0:
		close(fds.fd);
		reset_transfer_socket();
		return 0;

	default:
		return handle_ready_fds();
	}
}

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

void *udp_client_loop(__attribute__((unused)) void *arg)
{
	int8_t err;

	while (1) {
		int sfd = wait_transfer();

		fds.fd = sfd;
		fds.events = POLLIN;

		while (1) {
			err = handle_download();
			if (err == -1)
				return NULL;
			else if (err == 0)
				break;
		}
	}
}

/* -------------------------------------------------------------------------- */
