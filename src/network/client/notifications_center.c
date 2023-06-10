#include "network/client/notifications_center.h"

#include <poll.h>

#include "network/client/data.h"
#include "network/client/network.h"

static Array           m_fds;

/* ---------------------------- PRIVATE FUNCTIONS --------------------------- */

static uint8_t recv_notifications(void)
{
	struct pollfd *fds = m_fds.data;
	ServerRQ serverrq;
	ssize_t nbytes;

	for (size_t i = 0; i < m_fds.length; i++) {
	        if (!(fds[i].revents & POLLIN))
	        	continue;

	        while (1) {
	        	nbytes = recv_notif(fds[i].fd, &serverrq);
	        	if (nbytes < 0 && SBLOCK)
	                	break;

	        	if (nbytes < 0)
	        		return 1;

			if (add_notif(fds[i].fd, serverrq.nt.data, serverrq.nt.pseudo))
				return 1;
        	}
	}
	return 0;
}

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

void *notifications_center_loop(__attribute__((unused)) void *arg)
{
	uint8_t active = 1;
	int timeout = 1000 * 60;

    	wait_notif();

	if (array_new(&m_fds, sizeof(struct pollfd), 0))
        	return NULL;

    	while (active) {
        	refresh_fds(&m_fds);
        	switch (poll(m_fds.data, (nfds_t) m_fds.length, timeout)) {
	        case -1:
        		active = 0;
        		break;

        	case 0:
        		break;

        	default:
        		if (recv_notifications())
	    			return NULL;
            		break;
        	}
    	}

	array_free(&m_fds);
	return NULL;
}

/* -------------------------------------------------------------------------- */
