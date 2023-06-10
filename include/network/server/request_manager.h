/**
 * @file request_manager.h
 * @brief Contains prototypes for handling requests from clients.
 */

#ifndef REQUEST_MANAGER_H
#define REQUEST_MANAGER_H

/* -------------------------------- INCLUDE --------------------------------- */

#include "network/request.h"
#include "data_structures/array.h"

/* -------------------------------- FUNCTION -------------------------------- */

/**
 * @brief Gère la requete TCP en fonction de son type,
 * 	  et rempli la requete du serveur pour sa reponse
 */
uint8_t handle_tcp_request(Array *a_serverrq, ClientRQ *clientrq);

/**
 * @brief Gère les paquets udp reçu
 */
uint8_t handle_upload_packet(ClientRQ *clientrq, size_t nbytes);

/**
 * @brief Creer l'ensemble des paquets udp pour l'envoie d'un fichier,
 * 	  Stocket dans le tableau 'a_ftrq'
 */
uint8_t create_ftransfer_requests(Array *a_ftrq, size_t *f_size, uint16_t id);

/* -------------------------------------------------------------------------- */

#endif /* REQUEST_MANAGER_H */
