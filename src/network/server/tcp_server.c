#include "network/server/tcp_server.h"

#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "network/server/data.h"
#include "network/server/server.h"
#include "network/server/network.h"
#include "network/server/request_manager.h"

#include "system/logger.h"


static Server m_server;
static Array  m_fds;

typedef struct
{
	SA_IN6 addr;
	BytesRQ rb_rq;
} ConnectionInfos;

static Array  m_connection_infos;


static uint8_t handle_file_dowload(uint16_t id, uint16_t port, SA_IN6 *addr)
{
	Array a_ftrq;
	size_t file_size = 0;
	if (create_ftransfer_requests(&a_ftrq, &file_size, id))
		return 1;

	int ft_sfd = socket(DOMAIN, SOCK_DGRAM, 0);
	if (ft_sfd < 0)
		return 0;

	SA_IN6 ft_addr;
	memset(&ft_addr, 0, sizeof(ft_addr));
	ft_addr.sin6_family = DOMAIN;
	ft_addr.sin6_port = htons(port);
	ft_addr.sin6_addr = addr->sin6_addr;

	if (send_ftransfer_requests(ft_sfd, &ft_addr, &a_ftrq, file_size)) {
		debug_logerror("send_ftransfer_requests");
		id_clear_transfer(id);
		close(ft_sfd);
		return 1;
	}

	id_clear_transfer(id);
	array_free(&a_ftrq);
	close(ft_sfd);
	return 0;
}

static uint8_t server_callback(int sfd, ClientRQ *clientrq, ServerRQ *serverrq)
{
	ConnectionInfos *infos;
	coderq_t type = serverrq->type;
	if (type == DOWNLOAD) {
		infos = m_connection_infos.get(&m_connection_infos, (size_t) sfd);
		if (infos == NULL)
			return 1;

		uint16_t id = get_id(serverrq->cl.header);
		if (handle_file_dowload(id, clientrq->cl.count, &infos->addr))
			return 1;
	}

	return 0;
}

static uint8_t handle_tcp_connection(int client_sfd, uint8_t *close_connection)
{
	Array a_serverrq;
	memset(&a_serverrq, 0, sizeof(a_serverrq));
	if (array_new(&a_serverrq, sizeof(ServerRQ), 0)) {
		logerror("array_new: a_serverrq");
		return 1;
	}

	ServerRQ *serverrq = a_serverrq.data;

	ClientRQ clientrq;
	memset(&clientrq, 0, sizeof(clientrq));

	ConnectionInfos *infos = m_connection_infos.get(&m_connection_infos,
							(size_t) client_sfd);
	if (infos == NULL) {
		logerror("No connection infos");
		return 1;
	}


	if (recv_client_request(client_sfd, &clientrq, &infos->rb_rq)) {
		debug_logerror("recv_client_request");
		close(client_sfd);
		return 1;
	}

	debug_clientrq(&clientrq);

	if (handle_tcp_request(&a_serverrq, &clientrq)) {
		debug_logerror("handle_tcp_request");
		close(client_sfd);
		return 1;
	}

	debug_serverrq(serverrq);

	if (send_server_request(client_sfd, serverrq)) {
		debug_logerror("send_server_request");
		array_free(&a_serverrq);
		close(client_sfd);
		return 1;
	}

	uint16_t id = get_id(serverrq->cl.header);
	coderq_t rqtype = serverrq->type;
	log_to_file(LOG_REQUEST_FORMAT, strcoderq(rqtype), id, strerrno());

	if (server_callback(client_sfd, &clientrq, serverrq)) {
		debug_logerror("server_callback");
		return 1;
	}

	array_free(&a_serverrq);
	*close_connection = 1;
	return 0;
}

static void add_new_connections(int sfd, SA_IN6 *addr)
{
	struct pollfd *fds = m_fds.data;
	struct pollfd pfd = {.fd = sfd, .events = POLLIN };

	ConnectionInfos infos;
	memset(&infos, 0, sizeof(infos));

	infos.addr = *addr;
	m_connection_infos.set(&m_connection_infos, (size_t) sfd, &infos);
	for (size_t i = 0; i < m_fds.length; i++) {
		if (fds[i].fd == -1) {
			fds[i] = pfd;
			return;
		}
	}

	m_fds.append(&m_fds, &pfd);
}

static uint8_t accept_new_connections(void)
{
	SA_IN6 addr;
	memset(&addr, 0, sizeof(addr));

	int sfd;
	socklen_t addrlen = sizeof(addr);
	while (1) {
		sfd = accept(m_server.sfd, (SA *) &addr, &addrlen);
		if (sfd < 0 && SBLOCK)
			break;

		if (sfd < 0) {
			perror("accept");
			return 1;
		}

		add_new_connections(sfd, &addr);
	}

	return 0;
}

static uint8_t handle_ready_fds(void)
{
	struct pollfd *fds = m_fds.data;
	int current_size = (int) m_fds.length;

	for (int i = 0; i < current_size; i++) {
		if (!(fds[i].revents & POLLIN))
			continue;

		uint8_t is_tcpserver = (fds[i].fd == m_server.sfd);
		uint8_t is_tcpclient = (!is_tcpserver);

		/* Cas ou il y'a des connections entrantes. */
		if (is_tcpserver && accept_new_connections()) {
			logerror("accept_new_connections");
			return 1;
		}

		uint8_t cl_con = 0;
		/* Une socket TCP est prete pour la reception. */
		if (is_tcpclient && handle_tcp_connection(fds[i].fd, &cl_con)) {
			logerror("handle_tcp_connection");
			return 1;
		}

		ConnectionInfos *infos;
		infos = m_connection_infos.get(&m_connection_infos,
					       (size_t) fds[i].fd);
		if (cl_con && infos != NULL) {
			close(fds[i].fd);
			fds[i].fd = -1;
			memset(infos, 0, sizeof(ConnectionInfos));
		}
	}

	return 0;
}

void *tcp_server_loop(__attribute__((unused)) void *args)
{
	int timeout = 1000 * FT_TIMEOUT_SEC;
	uint8_t active = 1;

	while (active) {
		check_transfers_timeout();
		switch (poll(m_fds.data, (nfds_t) m_fds.length, timeout)) {
		case -1:
			perror("poll");
			active = 0;
			break;

		case 0:
			break;

		default:
			active = !handle_ready_fds();
			if (!active)
				logerror("handle_ready_fds");

			break;
		}
	}

	return NULL;
}

uint8_t tcp_server_init(in_port_t port)
{
	if (create_tcp_server(&m_server, port))
		return 1;

	if (array_new(&m_fds, sizeof(struct pollfd), ID_MAX))
		return 1;

	if (array_new(&m_connection_infos, sizeof(ConnectionInfos), ID_MAX))
		return 1;

	struct pollfd pfd = { .fd = m_server.sfd, .events = POLLIN };
	if (m_fds.append(&m_fds, &pfd)) {
		logerror("append: m_fds");
		return 1;
	}

	logsuccess("TCP Server Initialazed");
	return 0;
}
