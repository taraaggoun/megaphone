#ifndef REQUEST_MANAGER_H
#define REQUEST_MANAGER_H

/* -------------------------------- INCLUDE --------------------------------- */

#include "network/request.h"

/* -------------------------------- FUNCTION -------------------------------- */

uint8_t create_request(ClientRQ *clientrq, uint16_t id);
uint8_t create_ftransfer_requests(Array *a_ftrq, size_t *f_size, uint16_t id);

int8_t handle_download_packet(ServerRQ *serverrq, size_t nbytes);

/* -------------------------------------------------------------------------- */

#endif /* REQUEST_MANAGER_H */
