#ifndef NETWORK_H
#define NETWORK_H

/* -------------------------------- INCLUDE --------------------------------- */

#include "network/request.h"
#include "data_structures/array.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

/* -------------------------------- FUNCTIONS ------------------------------- */

/**
 * @brief Sends a server request.
 *
 * This function sends a server request to the specified socket.
 * The request is specified by the given ServerRQ structure and rq_type.
 *
 * @param sockfd The socket to send the request to.
 * @param serverrq The ServerRQ structure specifying the request to send.
 * @return 0 if the request is sent successfully, 1 otherwise.
 */
uint8_t send_server_request(int sfd, ServerRQ *serverrq);

uint8_t recv_client_request(int sfd, ClientRQ *clientrq, BytesRQ *rb_rq);

ssize_t recv_datagrams(int sfd, ClientRQ *clientrq);

uint8_t send_ftransfer_requests(int sfd, SA_IN6 *addr,
				Array *a_ftrq, size_t f_size);

uint8_t send_notif(int sfd, ServerRQ *serverrq, int nb_rq, SA_IN6 *sock_addr);

/* -------------------------------------------------------------------------- */

#endif /* NETWORK_H */
