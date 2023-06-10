#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <stdint.h>
#include <limits.h>
#include <time.h>

#include "data_structures/array.h"
#include "network/request_macros.h"
#include "network/network_macros.h"


#define UPLOAD_FILES_PATH "res/server/files"

#define FT_TIMEOUT_SEC    5

/* -------------------------------- STRUCTURES ------------------------------ */

typedef struct
{
	uint8_t active;               /* Transfer encours */
	time_t begin;                 /* Heure du debut du transfer */

	int sfd;
	SA_IN6 addr;

	char file_path[MAX_DATALEN];
	uint16_t feed_number;

	Array file_data;              /* Fichier en cours de téléchargement */
	uint16_t last_packet;         /* Numero du dernier paquet */
	off_t file_size;
} FileTransferInfos;

/* -------------------------------- FUNCTIONS ------------------------------- */

FileTransferInfos transfer_init(const char *file_name, uint16_t feed_number);

void transfer_clear(FileTransferInfos *infos);

uint8_t file_exist(char *file_name, uint16_t feed_number);

/* -------------------------------------------------------------------------- */

#endif /* FILE_TRANSFER_H */
