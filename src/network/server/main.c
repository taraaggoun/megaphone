/**
 * @file main.c
 * @brief Main entry point for the server application.
 */

#include <stdlib.h>

#include "network/server/server_manager.h"

/* --------------------------------- MAIN ----------------------------------- */

/**
 * @brief Simply calls the 'launch_server' function
 * 	  from 'server_manager' module.
 *
 * @return Program exited with an error.
 */
int main(int argc, const char *argv[])
{
	launch_server(argc, argv);

	return EXIT_FAILURE;
}

/* -------------------------------------------------------------------------- */
