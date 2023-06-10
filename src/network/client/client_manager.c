#include "network/client/client_manager.h"

#include <stdlib.h>
#include <string.h>

#include "user/user.h"

#include "network/network_macros.h"

#include "network/client/client.h"
#include "network/client/data.h"

#include "network/client/tcp_client.h"
#include "network/client/udp_client.h"
#include "network/client/notifications_center.h"

#include "system/logger.h"
#include "system/thread_pool.h"


/* ---------------------------- PRIVATE FUNCTIONS --------------------------- */

static uint8_t is_port(const char *port)
{
	char *endptr;
	int port_min = 1024;
	int port_max = 49151;
	long l = strtol(port, &endptr, 10);
	if (endptr == port || endptr[0] != 0) {
		logerror("The port is not an integer");
		return 0;
	}

	if (l < port_min || l > port_max) {
		logerror("The port is not an integer");
		return 0;
	}

	return 1;
}

/*
 * -i adresse ip | -p port
 */
static void parse(int argc, const char *argv[], char *hostname, char *port)
{
	memset(port, 0, PORT_STRLEN);
	strcpy(port, TCP_PORT_STR);

	memset(hostname, 0, HOSTNAME_STRLEN);
	strcpy(hostname, "::1");

	if (argc == 1)
		return;
	if (argc == 3 && !strcmp(argv[1], "-p")) {
		if (is_port(argv[2]) ) {
			memset(port, 0, PORT_STRLEN);
			strcpy(port, argv[2]);
		}
		else
			exit(EXIT_FAILURE);
		return;
	}

	if (argc == 3 && !strcmp(argv[1], "-i")) {
		memset(hostname, 0, HOSTNAME_STRLEN);
		strcpy(hostname, argv[2]);
		return;
	}

	if (argc == 5) {
		uint8_t id_p = 0;
		uint8_t id_i = 0;
		if(!strcmp(argv[1], "-p"))
			id_p = 2;
		if(!strcmp(argv[1], "-i"))
			id_i = 2;
		if(!strcmp(argv[3], "-p"))
			id_p = 4;
		if(!strcmp(argv[3], "-i"))
			id_i = 4;

		memset(hostname, 0, HOSTNAME_STRLEN);
		strcpy(hostname, argv[id_i]);
		if (is_port(argv[id_p])) {
			memset(port, 0, PORT_STRLEN);
			strcpy(port, argv[id_p]);
		}
		else
			exit(EXIT_FAILURE);
		if (id_i != 0 && id_p != 0)
			return;
	}
	logerror("format incorrect\n Please put -i before IP address or the hostname and -p before the port");
	
	exit(EXIT_FAILURE);
}

/* ---------------------------- PUBLIC FUNCTION ----------------------------- */

void launch_client(int argc, const char *argv[])
{
	const uint8_t thread_count = 3;

	ThreadPool *thread_pool;
	ThreadJob jobs[thread_count];

	char port[PORT_STRLEN];
	char hostname[HOSTNAME_STRLEN];

	/* Initialisations */
	data_init();
	if (load_accounts())
		exit(EXIT_FAILURE);

	parse(argc, argv, hostname, port);
	tcp_client_init(port);
	set_hostname(hostname);

	thread_pool = thread_pool_init(thread_count);
	if (thread_pool == NULL)
		exit(EXIT_FAILURE);

	memset(jobs, 0, sizeof(jobs));
	jobs[0].job = tcp_client_loop;
	jobs[1].job = udp_client_loop;
	jobs[2].job = notifications_center_loop;

	for (int i = 0; i < thread_count; i++)
		thread_pool->add_job(thread_pool, &jobs[i]);

	thread_pool_wait(thread_pool);
	thread_pool_shutdown(thread_pool);

	unload_accounts();
}

/* -------------------------------------------------------------------------- */
