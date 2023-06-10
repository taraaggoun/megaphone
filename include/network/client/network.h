#ifndef NETWORK_H
#define NETWORK_H


/* -------------------------------- INCLUDES -------------------------------- */

#include "network/request.h"
#include "network/client/client.h"

/* -------------------------------- FUNCTIONS ------------------------------- */

uint8_t send_client_request(Client *client, ClientRQ *clientrq);
uint8_t send_ftransfer_requests(Client *client, Array *a_ftrq, size_t f_size);
uint8_t recv_server_request(int sfd, ServerRQ **s_rq);
ssize_t recv_notif(int fd, ServerRQ *rq);
ssize_t recv_datagrams(int sfd, ServerRQ *serverrq);

/* -------------------------------------------------------------------------- */

#endif /* NETWORK_H */
