/**
 * @file server_manager.h
 * @brief Implementation of functions used for managing a server.
 */

#include "network/server/server_manager.h"

#include <stdlib.h>
#include <string.h>

#include "network/network_macros.h"

#include "network/server/data.h"

#include "network/server/tcp_server.h"
#include "network/server/udp_server.h"
#include "network/server/notifications_server.h"

#include "system/logger.h"
#include "system/thread_pool.h"


static uint8_t is_port(const char *portstr, uint16_t *port){
	char *endptr;
	int port_min = 1024;
	int port_max = 49151;
	long l = strtol(portstr, &endptr, 10);
	if (endptr == portstr || endptr[0] != 0) {
		logerror("The port is not an integer");
		return 0;
	}
	if (l < port_min || l > port_max) {
		logerror("The port is not an integer");
		return 0;
	}
	*port = (uint16_t) l;
	return 1;
}

/*
 * -t port tcp
 * -u port udp
*/
static void parse(int argc, const char *argv[], uint16_t *port_tcp, uint16_t *port_udp)
{
	if (argc == 1)
		return;
	if (argc == 3 && !strcmp(argv[1], "-t")) {
		if (!is_port(argv[2], port_tcp))
			exit(EXIT_FAILURE);
		return;
	}

	if (argc == 3 && !strcmp(argv[1], "-u")) {
		if (!is_port(argv[2], port_udp))
			exit(EXIT_FAILURE);
		return;
	}

	if (argc == 5) {
		uint8_t id_t = 0;
		uint8_t id_u = 0;
		if(!strcmp(argv[1], "-t"))
			id_t = 2;
		if(!strcmp(argv[1], "-u"))
			id_u = 2;
		if(!strcmp(argv[3], "-t"))
			id_t = 4;
		if(!strcmp(argv[3], "-u"))
			id_u = 4;

		if (!is_port(argv[2], port_tcp))
			exit(EXIT_FAILURE);

		if (!is_port(argv[2], port_udp))
			exit(EXIT_FAILURE);

		if (id_t != 0 && id_u != 0)
			return;
	}
	logerror("format incorrect\n Please put -t before TCP port and -u before UDP port");
	exit(EXIT_FAILURE);
}

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */


void launch_server(int argc, const char *argv[])
{
	const uint8_t thread_count = 3;

	ThreadPool *thread_pool;
	ThreadJob jobs[thread_count];

	uint16_t tcp_port = TCP_PORT;
	uint16_t udp_port = UDP_PORT;

	/* ------ initialization ------ */
	log_init();
	data_init();

	parse(argc, argv, &tcp_port, &udp_port);
	if (tcp_server_init(tcp_port)) {
		exit(EXIT_FAILURE);
	}
	if (udp_server_init(udp_port)) {
		exit(EXIT_FAILURE);
	}
	/* ---------------------------- */

	thread_pool = thread_pool_init(thread_count);
	if (thread_pool == NULL)
		exit(EXIT_FAILURE);

	memset(jobs, 0, sizeof(jobs));
	jobs[0].job = tcp_server_loop;
	jobs[1].job = udp_server_loop;
	jobs[2].job = notifications_loop;

	for (int i = 0; i < thread_count; i++)
		thread_pool->add_job(thread_pool, &jobs[i]);

	thread_pool_wait(thread_pool);
	thread_pool_shutdown(thread_pool);

	data_free();
	log_close();

	exit(EXIT_FAILURE);
}

/* -------------------------------------------------------------------------- */
