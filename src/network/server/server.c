/**
 * @file server.c
 * @brief Implementation of functions necessary to initialize and
 * 	  shutdown a server in a network communication context.
 */

#include "network/server/server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

/* ---------------------------- PRIVATE FUNCTIONS --------------------------- */

/**
 * @brief Sets socket options for a given socket file descriptor.
 *
 * The socket can accept both IPv4 and IPv6 connections.
 * The address can be reused immediately after the socket is closed.
 *
 * @param sockfd File descriptor the socket.
 * @return 0 on success and 1 on failure.
 */
static uint8_t sockopt_init(int sockfd)
{
	int optv;
	int optr;

	socklen_t s_optv = sizeof(optv);

	optv = 0;
	optr = setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &optv, s_optv);
	if (optr < 0) {
		perror("setsockopt: IPV6_V6ONLY");
		return 1;
	}

	optv = 1;
	optr = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optv, s_optv);
	if (optr < 0) {
		perror("setsockopt: SO_REUSEADDR");
		return 1;
	}

	return 0;
}

static uint8_t set_non_blocking(int sockfd)
{
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags < 0)
		return 1;

	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
		return 1;

	return 0;
}

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

uint8_t create_tcp_server(Server *tcp_server, in_port_t port)
{
	tcp_server->port = port;
	tcp_server->domain = DOMAIN;
	tcp_server->service = SOCK_STREAM;

	tcp_server->sfd = socket(tcp_server->domain, tcp_server->service, 0);
	if (tcp_server->sfd < 0) {
		perror("tcp socket");
		return 1;
	}

	/* Setting non blocking */
	if (set_non_blocking(tcp_server->sfd)) {
		perror("tcp set_non_blocking");
		return 1;
	}

	SA_IN6 addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin6_family = tcp_server->domain;
	addr.sin6_port = htons(tcp_server->port);
	addr.sin6_addr = in6addr_any;

	/* Changing socket options */
	if (sockopt_init(tcp_server->sfd))
		return 1;

	if (bind(tcp_server->sfd, (SA *) &addr, sizeof(addr)) < 0) {
		perror("tcp bind");
		return 1;
	}

	tcp_server->addr = addr;
	if (listen(tcp_server->sfd, 0) < 0) {
		perror("listen");
		return 1;
	}

	return 0;
}

uint8_t create_udp_server(Server *udp_server, in_port_t port)
{
	udp_server->port = port;
	udp_server->domain = DOMAIN;
	udp_server->service = SOCK_DGRAM;

	udp_server->sfd = socket(udp_server->domain, udp_server->service, 0);
	if (udp_server->sfd < 0) {
		perror("udp socket");
		return 1;
	}

	SA_IN6 addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin6_family = udp_server->domain;
	addr.sin6_port = htons(udp_server->port);
	addr.sin6_addr = in6addr_any;

	/* -- Changing socket options -- */
	if (sockopt_init(udp_server->sfd))
		return 1;

	if (bind(udp_server->sfd, (SA *) &addr, sizeof(addr)) < 0) {
		perror("udp bind");
		return 1;
	}

	udp_server->addr = addr;
	return 0;
}

/* -------------------------------------------------------------------------- */
