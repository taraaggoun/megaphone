#ifndef REQUEST_H
#define REQUEST_H

/* -------------------------------- INCLUDE --------------------------------- */

#include <stdio.h>
#include <sys/types.h>
#include <limits.h>

#include "data_structures/array.h"

#include "network_macros.h"
#include "request_macros.h"
#include "user/user.h"

/* --------------------------------- DEFINES -------------------------------- */

#define CODERQ_BITSLEN      5

#define REGISTRATION        0x01
#define NEWPOST             0x02
#define LASTPOSTS           0x03
#define SUBSCRIBE           0x04
#define UPLOAD              0x05
#define DOWNLOAD            0x06

typedef uint8_t             coderq_t;
typedef uint16_t	    header_t;
typedef char		    rq_buf_t[BUFSIZ];

#define LOG_REQUEST_FORMAT  "CODERQ:%-12s, USERID:%u, ERROR:%s\n"

extern char CRLF[3];

/* -------------------------------- STRUCTURES ------------------------------ */

typedef struct
{
	header_t header;
	uint16_t numblock;

	char data[FILE_PACKET_SIZE];
} FTransferRQ;

/* ------ Client requests ------ */

typedef struct
{
	header_t header;
	pseudo_t pseudo;
} ClientRQ_Rg;

typedef struct
{
	header_t header;
	uint16_t feed_number;

	uint16_t count;

	/* 0 <= datalen <= MAX_DATALEN = 255 */
	uint8_t datalen;
	char data[MAX_DATALEN];
} ClientRQ_Cl;

typedef struct
{
	coderq_t type;
	union
	{
		ClientRQ_Rg rg;  /* Registration request */
		ClientRQ_Cl cl;  /* Other request */
		FTransferRQ ft;  /* File Transfer request */
	};
} ClientRQ;

/* ------ Server requests ------ */

#define NT_DATA_LEN   20
#define ADDRMULT_LEN  16

typedef struct
{
	header_t header;
	uint16_t feed_number;
	uint16_t count;
} ServerRQ_Cl;

typedef struct
{
	uint16_t feed_number;

	pseudo_t creator;
	pseudo_t pseudo;

	uint8_t datalen;
	char data[MAX_DATALEN];
} ServerRQ_Lp;

typedef struct
{
	header_t header;
	uint16_t feed_number;

	pseudo_t pseudo;
	char data[NT_DATA_LEN];
} ServerRQ_Nt;

typedef struct
{
	header_t header;
	uint16_t feed_number;
	uint16_t count;

	char addr[ADDRMULT_LEN];
} ServerRQ_Sb;

typedef struct
{
	coderq_t type;
	union
	{
		ServerRQ_Cl cl;   /* Registration, Post and Error */
		ServerRQ_Sb sb;   /* Subscribe */
		ServerRQ_Lp lp;   /* LastPosts */
		ServerRQ_Nt nt;   /* Notifications */
		FTransferRQ ft;   /* File Transfer request */
	};
} ServerRQ;

/* ------ Other structs ------ */

typedef struct
{
	char addr[ADDRMULT_LEN];
	uint16_t port;
	int sock_fd;
	SA_IN6 sock_addr;
} Mult;

typedef struct
{
	size_t nbfeed;
	size_t last_send_post;
	Mult mult_infos;
} NotificationsInfos;

/**
 * @brief Permet de stocker une requete dans un buffer
 */
typedef struct
{
	rq_buf_t buf;
	size_t   size;
} BytesRQ;

/* -------------------------------- FUNCTIONS ------------------------------- */

/* --------- Requests init --------- */

ServerRQ_Lp serverrq_lp_new(uint16_t feed_number, pseudo_t *creator,
			    pseudo_t *pseudo, uint8_t datalen, char *data);

ServerRQ_Nt serverrq_nt_new(uint16_t hd, uint16_t feed_number,
			    pseudo_t *pseudo, char *data);

ServerRQ_Cl serverrq_error(void);

Array ftransferrqs_new(header_t header, char *file, off_t file_size);

/* -------------------------------- */

/**
 * @brief Permer de recevoir une requete TCP,
 *        La stocke dans 'bytes_rq' et le reste de la requete dans 'rbytes_rq'
 */
uint8_t recv_request(int sfd, BytesRQ *bytes_rq, BytesRQ *rbytes_rq);

/**
 * @brief Extrait le type de la requete depuis son header
 */
coderq_t get_rq_type(header_t hd);

/**
 * @brief Extrait l'id de la requete depuis son header
 */
uint16_t get_id(header_t hd);

/**
 * @brief Rajoute un element dans un buffer et mets a jour sa taille
 */
void appendbuf(rq_buf_t buf, size_t *buf_size, void *elt, size_t elt_size);

/**
 * @brief Extrait un element d'un buffer et mets a jour sa taille
 */
void extractbuf(char *buf, size_t *buf_size, void *elt, size_t elt_size);

/* --------- Debug --------- */

void debug_clientrq(ClientRQ *clientrq);

void debug_serverrq(ServerRQ *serverrq);

const char* strcoderq(coderq_t rq_type);

/* ------------------------- */

/* -------------------------------------------------------------------------- */

#endif /* REQUEST_H */
