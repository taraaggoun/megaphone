/**
 * @file network.c
 * @brief Implementation of the network module.
 */

#include "network/server/network.h"

#include <stdio.h>
#include <string.h>

#include "network/network_macros.h"
#include "system/logger.h"

/* ---------------------------- PRIVATE FUNCTIONS --------------------------- */


static BytesRQ hton_serverrqcl(ServerRQ_Cl *rq)
{
	BytesRQ b_rq;
	header_t hd = htons(rq->header);
	uint16_t feed_number = htons(rq->feed_number);
	uint16_t count = htons(rq->count);

	memset(&b_rq, 0, sizeof(b_rq));
	appendbuf(b_rq.buf, &b_rq.size, &hd, sizeof(hd));
	appendbuf(b_rq.buf, &b_rq.size, &feed_number, sizeof(feed_number));
	appendbuf(b_rq.buf, &b_rq.size, &count, sizeof(count));

	return b_rq;
}


static BytesRQ hton_serverrqlp(ServerRQ_Lp *rq)
{
	BytesRQ b_rq;
	uint16_t feed_number = htons(rq->feed_number);

	memset(&b_rq, 0, sizeof(b_rq));
	appendbuf(b_rq.buf, &b_rq.size, &feed_number, sizeof(feed_number));
	appendbuf(b_rq.buf, &b_rq.size, rq->creator, PSEUDO_LEN);
	appendbuf(b_rq.buf, &b_rq.size, rq->pseudo, PSEUDO_LEN);
	appendbuf(b_rq.buf, &b_rq.size, &rq->datalen, sizeof(rq->datalen));
	appendbuf(b_rq.buf, &b_rq.size, rq->data, rq->datalen);

	return b_rq;
}

static BytesRQ hton_serverrqft(FTransferRQ *rq)
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
static BytesRQ hton_serverrqsb(ServerRQ_Sb *rq)
{
	BytesRQ b_rq;
	header_t hd = htons(rq->header);
	uint16_t feed_number = htons(rq->feed_number);
	uint16_t count = htons(rq->count);

	memset(&b_rq, 0, sizeof(b_rq));
	appendbuf(b_rq.buf, &b_rq.size, &hd, sizeof(hd));
	appendbuf(b_rq.buf, &b_rq.size, &feed_number, sizeof(feed_number));
	appendbuf(b_rq.buf, &b_rq.size, &count, sizeof(count));
	appendbuf(b_rq.buf, &b_rq.size, rq->addr, sizeof(rq->addr));

	return b_rq;
}

static BytesRQ hton_serverrqnt(ServerRQ_Nt *rq)
{
	BytesRQ b_rq;
	header_t hd = htons(rq->header);
	uint16_t feed_number = htons(rq->feed_number);

	memset(&b_rq, 0, sizeof(b_rq));
	appendbuf(b_rq.buf, &b_rq.size, &hd, sizeof(hd));
	appendbuf(b_rq.buf, &b_rq.size, &feed_number, sizeof(feed_number));
	appendbuf(b_rq.buf, &b_rq.size, rq->pseudo, PSEUDO_LEN);
	appendbuf(b_rq.buf, &b_rq.size, rq->data, NT_DATA_LEN);

	return b_rq;
}

static ClientRQ_Rg ntoh_clientrqrg(BytesRQ *b_rq)
{
	ClientRQ_Rg rq;
	uint16_t hd;

	size_t size = 0;
	extractbuf(b_rq->buf, &size, &hd, sizeof(hd));
	extractbuf(b_rq->buf, &size, rq.pseudo, PSEUDO_LEN);

	rq.header = ntohs(hd);
	return rq;
}


static ClientRQ_Cl ntoh_clientrqcl(BytesRQ *b_rq)
{
	ClientRQ_Cl rq;
	memset(&rq, 0, sizeof(rq));
	header_t header;
	uint16_t feed_number;
	uint16_t count;

	size_t size = 0;
	extractbuf(b_rq->buf, &size, &header, sizeof(header));
	extractbuf(b_rq->buf, &size, &feed_number, sizeof(feed_number));
	extractbuf(b_rq->buf, &size, &count, sizeof(count));
	extractbuf(b_rq->buf, &size, &rq.datalen, sizeof(rq.datalen));
	extractbuf(b_rq->buf, &size, rq.data, rq.datalen);

	rq.header = ntohs(header);
	rq.feed_number = ntohs(feed_number);
	rq.count = ntohs(count);
	return rq;
}

static FTransferRQ ntoh_clientrqup(BytesRQ *bytes_rq)
{
	FTransferRQ rq;
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

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

uint8_t send_server_request(int sfd, ServerRQ *serverrq)
{
        BytesRQ bytes_rq;
	coderq_t type = serverrq->type;

	if (type == REGISTRATION || type == NEWPOST || type == UPLOAD ||
	    type == DOWNLOAD || is_error(type)) {
		bytes_rq = hton_serverrqcl(&serverrq->cl);
		appendbuf(bytes_rq.buf, &bytes_rq.size, CRLF, strlen(CRLF));
		if (send(sfd, bytes_rq.buf, bytes_rq.size, 0) < 0) {
			perror("send");
	                return 1;
		}
	} else if (type == LASTPOSTS) {
		bytes_rq = hton_serverrqcl(&serverrq->cl);
		appendbuf(bytes_rq.buf, &bytes_rq.size, CRLF, strlen(CRLF));
		if (send(sfd, bytes_rq.buf, bytes_rq.size, 0) < 0) {
			perror("send");
			return 1;
		}
		for (uint16_t i = 0; i < serverrq->cl.count; i++) {
			memset(&bytes_rq, 0, BUFSIZ);
			bytes_rq = hton_serverrqlp(&serverrq[i + 1].lp);
			appendbuf(bytes_rq.buf, &bytes_rq.size, CRLF, strlen(CRLF));
			if (send(sfd, bytes_rq.buf, bytes_rq.size, 0) < 0) {
				perror("send");
				return 1;
			}
		}
	} else if (type == SUBSCRIBE) {
		bytes_rq = hton_serverrqsb(&serverrq->sb);
		appendbuf(bytes_rq.buf, &bytes_rq.size, CRLF, strlen(CRLF));
		if (send(sfd, bytes_rq.buf, bytes_rq.size, 0) < 0) {
			perror("send");
	                return 1;
		}
	} else {
		logerror("NOT IMPLEMENTED");
		return 1;
	}

	return 0;
}

uint8_t recv_client_request(int sockfd, ClientRQ *clientrq, BytesRQ *rb_rq)
{
	BytesRQ b_rq;

	if (recv_request(sockfd, &b_rq, rb_rq))
                return 1;

	header_t hd;
	extractbuf(b_rq.buf, NULL, &hd, sizeof(hd));
	clientrq->type = get_rq_type(ntohs(hd));

	if (clientrq->type == REGISTRATION)
		clientrq->rg = ntoh_clientrqrg(&b_rq);
	else
		clientrq->cl = ntoh_clientrqcl(&b_rq);

        return 0;
}

ssize_t recv_datagrams(int sfd, ClientRQ *clientrq)
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
	clientrq->type = get_rq_type(ntohs(hd));
	clientrq->ft = ntoh_clientrqup(&bytes_rq);

	return nbytes;
}

static uint8_t send_datagrams(int sfd, SA_IN6 *addr, FTransferRQ *ftrq, size_t blen)
{
	BytesRQ bytes_rq = hton_serverrqft(ftrq);
	socklen_t len = sizeof(*addr);

	bytes_rq.size -= (FILE_PACKET_SIZE - blen);
	if (sendto(sfd, bytes_rq.buf, bytes_rq.size, 0, (SA *) addr, len) < 0)
		return 1;

	return 0;
}


uint8_t send_ftransfer_requests(int sfd, SA_IN6 *addr, Array *a_ftrq, size_t f_size)
{
	FTransferRQ *ftrq = a_ftrq->data;
	for (size_t i = 0; i < a_ftrq->length; i++) {
		size_t len = FILE_PACKET_SIZE;
		if (i == a_ftrq->length - 1)
			len = (size_t) (f_size % FILE_PACKET_SIZE);

		if (send_datagrams(sfd, addr, ftrq + i, len))
			return 1;
	}

	return 0;
}

uint8_t send_notif(int sockfd, ServerRQ *serverrq, int nb_rq, SA_IN6 *sock_addr)
{
        BytesRQ bytes_rq;
	socklen_t len = sizeof(*sock_addr);

	for (int i = 0; i < nb_rq; i++) {
		memset(&bytes_rq, 0, sizeof(bytes_rq));
		bytes_rq = hton_serverrqnt(&serverrq[i].nt);
		if (sendto(sockfd, bytes_rq.buf, bytes_rq.size, 0, (SA *) sock_addr, len) < 0) {
			perror("send");
			return 1;
		}
	}
	return 0;
}

/* -------------------------------------------------------------------------- */
