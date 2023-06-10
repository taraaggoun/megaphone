#include "network/client/network.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "network/network_macros.h"

#include "system/logger.h"


/* ---------------------------- PRIVATE FUNCTIONS --------------------------- */

static BytesRQ hton_clientrqrg(ClientRQ_Rg *rq)
{
	BytesRQ b_rq;
	memset(&b_rq, 0, sizeof(b_rq));

	header_t hd = htons(rq->header);

	appendbuf(b_rq.buf, &b_rq.size, &hd, sizeof(hd));
	appendbuf(b_rq.buf, &b_rq.size, rq->pseudo, PSEUDO_LEN);

	return b_rq;
}

static BytesRQ hton_clientrqcl(ClientRQ_Cl *rq)
{
	BytesRQ b_rq;
	memset(&b_rq, 0, sizeof(b_rq));

	header_t hd = htons(rq->header);
	uint16_t feed_number = htons(rq->feed_number);
	uint16_t count = htons(rq->count);

	appendbuf(b_rq.buf, &b_rq.size, &hd, sizeof(hd));
	appendbuf(b_rq.buf, &b_rq.size, &feed_number, sizeof(feed_number));
	appendbuf(b_rq.buf, &b_rq.size, &count, sizeof(count));
	appendbuf(b_rq.buf, &b_rq.size, &rq->datalen, sizeof(rq->datalen));
	appendbuf(b_rq.buf, &b_rq.size, rq->data, rq->datalen);

	return b_rq;
}

static BytesRQ hton_clientrqft(FTransferRQ *rq)
{
	BytesRQ b_rq;
	memset(&b_rq, 0, sizeof(b_rq));

	header_t hd = htons(rq->header);
	uint16_t numbloc = htons(rq->numblock);

	appendbuf(b_rq.buf, &b_rq.size, &hd, sizeof(hd));
	appendbuf(b_rq.buf, &b_rq.size, &numbloc, sizeof(numbloc));
	appendbuf(b_rq.buf, &b_rq.size, rq->data, FILE_PACKET_SIZE);

	return b_rq;
}

static ServerRQ_Cl ntoh_serverrqcl(BytesRQ *b_rq)
{
	ServerRQ_Cl rq;
	memset(&rq, 0, sizeof(rq));

	header_t hd;
	uint16_t feed_number;
	uint16_t count;

	size_t size = 0;
	extractbuf(b_rq->buf, &size, &hd, sizeof(hd));
	extractbuf(b_rq->buf, &size, &feed_number, sizeof(feed_number));
	extractbuf(b_rq->buf, &size, &count, sizeof(count));

	rq.header = ntohs(hd);
	rq.feed_number = ntohs(feed_number);
	rq.count = ntohs(count);
	return rq;
}

static ServerRQ_Lp ntoh_serverrqlp(BytesRQ *b_rq)
{
	ServerRQ_Lp rq;
	uint16_t feed_number;

	size_t size = 0;
	extractbuf(b_rq->buf, &size, &feed_number, sizeof(feed_number));
	extractbuf(b_rq->buf, &size, rq.creator, PSEUDO_LEN);
	extractbuf(b_rq->buf, &size, rq.pseudo, PSEUDO_LEN);
	extractbuf(b_rq->buf, &size, &rq.datalen, sizeof(rq.datalen));
	extractbuf(b_rq->buf, &size, rq.data, rq.datalen);

	rq.feed_number = ntohs(feed_number);
	return rq;
}

static FTransferRQ ntoh_serverrqdl(BytesRQ *bytes_rq)
{
	FTransferRQ rq;
	memset(&rq, 0, sizeof(rq));

	header_t header;
	uint16_t numblock;

	size_t size = 0;
	extractbuf(bytes_rq->buf, &size, &header, sizeof(header));
	extractbuf(bytes_rq->buf, &size, &numblock, sizeof(numblock));
	extractbuf(bytes_rq->buf, &size, rq.data, bytes_rq->size - size);

	rq.header = ntohs(header);
	rq.numblock = ntohs(numblock);

	return rq;
}

static ServerRQ_Sb ntoh_serverrqsb(BytesRQ *b_rq)
{
	ServerRQ_Sb rq;
	header_t hd;
	uint16_t feed_number;
	uint16_t count;
	char addr[16];

	size_t size = 0;
	extractbuf(b_rq->buf, &size, &hd, sizeof(hd));
	extractbuf(b_rq->buf, &size, &feed_number, sizeof(feed_number));
	extractbuf(b_rq->buf, &size, &count, sizeof(count));
	extractbuf(b_rq->buf, &size, addr, sizeof(addr));

	rq.header = ntohs(hd);
	rq.feed_number = ntohs(feed_number);
	rq.count = ntohs(count);
	memcpy(rq.addr, addr, sizeof(addr));
	return rq;
}

static ServerRQ_Nt ntoh_serverrqnt(BytesRQ *b_rq)
{
	ServerRQ_Nt rq;
	header_t hd;
	uint16_t feed_number;

	size_t size = 0;
	extractbuf(b_rq->buf, &size, &hd, sizeof(hd));
	extractbuf(b_rq->buf, &size, &feed_number, sizeof(feed_number));
	extractbuf(b_rq->buf, &size, rq.pseudo, PSEUDO_LEN);
	extractbuf(b_rq->buf, &size, rq.data, NT_DATA_LEN);

	rq.header = ntohs(hd);
	rq.feed_number = ntohs(feed_number);
	return rq;
}

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

uint8_t send_client_request(Client *client, ClientRQ *clientrq)
{
	BytesRQ bytes_rq;

	if (clientrq->type == REGISTRATION)
		bytes_rq = hton_clientrqrg(&clientrq->rg);
	else
		bytes_rq = hton_clientrqcl(&clientrq->cl);

	appendbuf(bytes_rq.buf, &bytes_rq.size, CRLF, strlen(CRLF));
	if (send(client->sfd, bytes_rq.buf, bytes_rq.size, 0) < 0)
                return 1;

	return 0;
}

static uint8_t send_datagrams(Client *client, FTransferRQ *ftrq, size_t blen)
{
	BytesRQ bytes_rq = hton_clientrqft(ftrq);
	SA *addr = (SA *) &client->addr;
	socklen_t len = sizeof(client->addr);

	bytes_rq.size -= (FILE_PACKET_SIZE - blen);
	if (sendto(client->sfd, bytes_rq.buf, bytes_rq.size, 0, addr, len) < 0)
		return 1;

	return 0;
}

uint8_t send_ftransfer_requests(Client *client, Array *a_ftrq, size_t f_size)
{
	FTransferRQ *ftrq = a_ftrq->data;
	for (size_t i = 0; i < a_ftrq->length; i++) {
		size_t len = FILE_PACKET_SIZE;
		if (i == a_ftrq->length - 1)
			len = (size_t) (f_size % FILE_PACKET_SIZE);

		if (send_datagrams(client, ftrq + i, len))
			return 1;
	}

	return 0;
}


uint8_t recv_server_request(int sfd, ServerRQ **s_rq)
{
	static BytesRQ rb_rq = { 0 };

	BytesRQ b_rq;

        if (recv_request(sfd, &b_rq, &rb_rq))
		return 1;

	header_t hd;
	extractbuf(b_rq.buf, NULL, &hd, sizeof(hd));

	if ((*s_rq = calloc(1, sizeof(**s_rq))) == NULL)
		return 1;

	(*s_rq)->type = get_rq_type(ntohs(hd));
	if (is_error((*s_rq)->type))
		d_errno = (*s_rq)->type;

	if ((*s_rq)->type == REGISTRATION || (*s_rq)->type == NEWPOST ||
           (*s_rq)->type == UPLOAD || (*s_rq)->type == DOWNLOAD ||
	   is_error((*s_rq)->type)) {
		(*s_rq)->cl = ntoh_serverrqcl(&b_rq);
                return 0;
	} else if ((*s_rq)->type == LASTPOSTS) {
		(*s_rq)->cl = ntoh_serverrqcl(&b_rq);
		size_t count = (size_t) (*s_rq)->cl.count;
		*s_rq = realloc(*s_rq, sizeof(**s_rq) * (1 + count));
		if (*s_rq == NULL)
			return 1;

		for (size_t i = 0; i < count; i++) {
			memset(b_rq.buf, 0, BUFSIZ + 1);
			if (recv_request(sfd, &b_rq, &rb_rq))
				return 1;

			(*s_rq)[i + 1].lp = ntoh_serverrqlp(&b_rq);
		}
		return 0;
        } else if ((*s_rq)->type == SUBSCRIBE) {
		(*s_rq)->sb = ntoh_serverrqsb(&b_rq);
		return 0;
	}

	return 1;
}

ssize_t recv_notif(int fd, ServerRQ *rq)
{
	BytesRQ bytes_rq;
	memset(&bytes_rq, 0, sizeof(bytes_rq));

	ssize_t nbytes = recvfrom(fd, bytes_rq.buf, sizeof(FTransferRQ), 0, NULL, NULL);

	if (nbytes < 0)
		return nbytes;

	bytes_rq.size = (size_t) nbytes;

	header_t hd;
	extractbuf(bytes_rq.buf, NULL, &hd, sizeof(hd));
	rq->type = get_rq_type(ntohs(hd));
	rq->nt = ntoh_serverrqnt(&bytes_rq);

	return nbytes;
}

ssize_t recv_datagrams(int sfd, ServerRQ *serverrq)
{
	BytesRQ bytes_rq;
	memset(&bytes_rq, 0, sizeof(bytes_rq));

	ssize_t nbytes = recvfrom(sfd, bytes_rq.buf, sizeof(FTransferRQ), 0,
				  NULL, NULL);
	if (nbytes < 0)
		return nbytes;

	bytes_rq.size = (size_t) nbytes;

	header_t hd;
	extractbuf(bytes_rq.buf, NULL, &hd, sizeof(hd));
	serverrq->type = get_rq_type(ntohs(hd));
	serverrq->ft = ntoh_serverrqdl(&bytes_rq);

	return nbytes;
}

/* -------------------------------------------------------------------------- */
