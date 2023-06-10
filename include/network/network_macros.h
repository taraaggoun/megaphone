#ifndef NETWORK_MACROS_H
#define NETWORK_MACROS_H

/* -------------------------------- INCLUDES -------------------------------- */

#include <netdb.h>
#include <errno.h>

/* --------------------------------- DEFINES -------------------------------- */

#define TCP_PORT	 6226
#define TCP_PORT_STR	 "6226"

#define UDP_PORT 	 6227

#define DOMAIN           AF_INET6

#define PORT_STRLEN	 6
#define HOSTNAME_STRLEN  20

#define SBLOCK (errno == EWOULDBLOCK || errno == EAGAIN)

typedef struct sockaddr_in6 SA_IN6;
typedef struct sockaddr     SA;

/* -------------------------------------------------------------------------- */

#endif /* NETWORK_MACROS_H */
