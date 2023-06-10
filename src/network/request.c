#include "network/request.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#include "network/network_macros.h"

#include "system/logger.h"

static const char *strrq[] = {
	"Registration",
	"New post",
	"Last posts",
	"Subscribe",
	"Upload",
	"Download"
};

char CRLF[3] = "\r\n";

/* --------------------------- PRIVATE FUNCTIONS ---------------------------- */

/* ----- Client requests printer ----- */

static void debug_clientrqrg(ClientRQ_Rg *rq)
{
	debug_log("CODEREQ: %s", strcoderq(get_rq_type(rq->header)));
	debug_log("     ID: %u", get_id(rq->header));
	debug_log(" PSEUDO: %.*s\n", PSEUDO_LEN, rq->pseudo);
}

static void debug_clientrqcl(ClientRQ_Cl *rq)
{
	debug_log("CODEREQ: %s", strcoderq(get_rq_type(rq->header)));
	debug_log("     ID: %u", get_id(rq->header));

	debug_log(" NUMFIL: %u", rq->feed_number);
	debug_log("     NB: %u", rq->count);
	debug_log("DATALEN: %u", rq->datalen);
	debug_log("   DATA: %.*s\n", rq->datalen, rq->data);
}

/* ----- Server requests printer ----- */

static void debug_serverrq_lp(ServerRQ_Lp *rq)
{
	debug_log(" NUMFIL: %u", rq->feed_number);

	debug_log("CREATOR: %.*s", PSEUDO_LEN, rq->creator);
	debug_log(" PSEUDO: %.*s", PSEUDO_LEN, rq->pseudo);

	debug_log("DATALEN: %u", rq->datalen);
	debug_log("   DATA: %.*s\n", rq->datalen, rq->data);
}


static void debug_serverrq_cl(ServerRQ_Cl *rq)
{
	debug_log("CODEREQ: %s", strcoderq(get_rq_type(rq->header)));
	debug_log("     ID: %u", get_id(rq->header));
	debug_log(" NUMFIL: %d", rq->feed_number);
	debug_log("     NB: %d\n", rq->count);
}

static void debug_serverrq_sb(ServerRQ_Sb *rq)
{
	debug_log("CODEREQ: %s", strcoderq(get_rq_type(rq->header)));
	debug_log("     ID: %u", get_id(rq->header));
	debug_log(" NUMFIL: %d", rq->feed_number);
	debug_log("   Port: %d", rq->count);
	debug_log("   ADDR: %s\n", rq->addr);
}

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

ServerRQ_Lp serverrq_lp_new(uint16_t feed_number, pseudo_t *creator,
			    pseudo_t *pseudo, uint8_t datalen, char *data)
{
	ServerRQ_Lp serverrq_lp;

	serverrq_lp.feed_number = feed_number;
	memcpy(serverrq_lp.creator, creator, PSEUDO_LEN);
	memcpy(serverrq_lp.pseudo, pseudo, PSEUDO_LEN);

	serverrq_lp.datalen = datalen;
	memcpy(serverrq_lp.data, data, datalen);

	return serverrq_lp;
}

ServerRQ_Nt serverrq_nt_new(uint16_t hd, uint16_t feed_number, pseudo_t *pseudo, char *data)
{
	ServerRQ_Nt serverrq_nt;

	serverrq_nt.header = hd;
	serverrq_nt.feed_number = feed_number;
	memcpy(serverrq_nt.pseudo, pseudo, PSEUDO_LEN);
	memcpy(serverrq_nt.data, data, NT_DATA_LEN);

	return serverrq_nt;
}

ServerRQ_Cl serverrq_error(void)
{
      ServerRQ_Cl serverrq;

      serverrq.header = (uint16_t) d_errno;
      serverrq.feed_number = 0;
      serverrq.count = 0;

      return serverrq;
}

/*
 * Création de tous les paquets UDP à envoyer
 * contenant tous les morceaux du fichier.
 */
Array ftransferrqs_new(header_t header, char *file, off_t file_size)
{
	uint16_t numblock = (uint16_t) (file_size / FILE_PACKET_SIZE) + 1;
	if ((file_size % FILE_PACKET_SIZE) == 0 && file_size != 0)
		numblock++;

	Array a_rqft;
	if (array_new(&a_rqft, sizeof(FTransferRQ), numblock))
		return a_rqft;

	FTransferRQ *ft_rq = a_rqft.data;
	for (uint16_t i = 0; i < numblock; i++) {
		size_t len = FILE_PACKET_SIZE;
		if (i == numblock - 1)
			len = (size_t) (file_size % FILE_PACKET_SIZE);

		ft_rq[i].header = header;
		ft_rq[i].numblock = i + 1;

		memset(ft_rq[i].data, 0, FILE_PACKET_SIZE);
		if (file_size == 0)
			continue;

		memcpy(ft_rq[i].data, file + (i * FILE_PACKET_SIZE), len);
	}

	a_rqft.length = numblock;
	return a_rqft;
}


uint8_t recv_request(int sfd, BytesRQ *bytes_rq, BytesRQ *rbytes_rq)
{
	*bytes_rq = *rbytes_rq;
	memset(rbytes_rq, 0, sizeof(*rbytes_rq));

	while (1) {
		char *end = memchr(bytes_rq->buf, CRLF[0], bytes_rq->size);
		if (end != NULL) {
			size_t startlen = (size_t) (end - bytes_rq->buf);
			size_t endlen = bytes_rq->size - startlen - 1;

			uint8_t is_crlf = (end[1] == CRLF[1]);
			if (is_crlf) {
				end++;
				endlen--;
			}

			memcpy(rbytes_rq->buf, end + 1, endlen);
			rbytes_rq->size = endlen;

			memset(bytes_rq->buf + startlen + is_crlf, 0, endlen);
			bytes_rq->size = startlen;
			return 0;
		}

		ssize_t read_size;
		size_t len = BUFSIZ - bytes_rq->size;

		read_size = recv(sfd, bytes_rq->buf + bytes_rq->size, len, 0);
		if (read_size < 0 && SBLOCK)
			return 0;

		if (read_size < 0) {
			perror("recv");
			return 1;
		}

		if (read_size == 0)
			return 0;

		bytes_rq->size += (size_t) read_size;
	}
}

void appendbuf(rq_buf_t buf, size_t *buf_size, void *elt, size_t elt_size)
{
	memcpy(buf + *buf_size, elt, elt_size);
	(*buf_size) += elt_size;
}

void extractbuf(char *buf, size_t *buf_size, void *elt, size_t elt_size)
{
	if (buf_size == NULL) {
		memcpy(elt, buf, elt_size);
	} else {
		memcpy(elt, buf + *buf_size, elt_size);
		(*buf_size) += elt_size;
	}
}


#define HEADER_MASK 0x1F

coderq_t get_rq_type(header_t hd)
{
	return hd & HEADER_MASK;
}


uint16_t get_id(header_t hd)
{
	return (hd - (hd & HEADER_MASK)) >> CODERQ_BITSLEN;
}

/* --------- Print functions --------- */

void debug_clientrq(ClientRQ *clientrq)
{
	if (clientrq->type == REGISTRATION)
		debug_clientrqrg(&clientrq->rg);
	else
		debug_clientrqcl(&clientrq->cl);
}

void debug_serverrq(ServerRQ *serverrq)
{
	coderq_t type = serverrq->type;

	if (type == REGISTRATION || type == NEWPOST ||
	    type == UPLOAD || type == DOWNLOAD || d_errno != NOERROR) {
		debug_serverrq_cl(&serverrq->cl);
	} else if (type == LASTPOSTS) {
		const int maxpostprint = 10;
                int count = (int) serverrq->cl.count;
                debug_serverrq_cl(&serverrq->cl);

		int postcount = (count > maxpostprint ? maxpostprint : count);
                for (int i = 0; i < postcount; i++)
                        debug_serverrq_lp(&serverrq[i + 1].lp);
        } else if (type == SUBSCRIBE)
		debug_serverrq_sb(&serverrq->sb);
}

const char* strcoderq(coderq_t rq_type)
{
	if ((rq_type < 1 || rq_type > 6) && d_errno == NOERROR)
		return NULL;

	if (d_errno != NOERROR)
		return strerrno();

	return strrq[rq_type - 1];
}

/* -------------------------------------------------------------------------- */
