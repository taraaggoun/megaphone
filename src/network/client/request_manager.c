#include "network/client/request_manager.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "user/user_input.h"
#include "network/client/data.h"
#include "network/network_macros.h"

#include "system/logger.h"


/* ---------------------------- PRIVATE FUNCTIONS --------------------------- */

static uint8_t create_rqrg(ClientRQ *clientrq)
{
	char pseudo[INPUT_SIZE];
	ssize_t pseudo_len = (ssize_t) pseudo_manager(pseudo);
	if (pseudo_len < 0)
		return 1;

	clientrq->rg.header = (REGISTRATION | 0 << CODERQ_BITSLEN);
	memset(clientrq->rg.pseudo, '#', PSEUDO_LEN);
	memcpy(clientrq->rg.pseudo, pseudo, (size_t) pseudo_len);
	return 0;
}

static uint8_t create_transfer_address(SA_IN6 *addr)
{
	int sfd;
	if ((sfd = socket(DOMAIN, SOCK_DGRAM, 0)) < 0)
		return 1;

	memset(addr, 0, sizeof(*addr));
	addr->sin6_family = DOMAIN;
	addr->sin6_port = 0;
	addr->sin6_addr = in6addr_any;

	int flags = fcntl(sfd, F_GETFL, 0);
	if (flags < 0)
		return 1;

	if (fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0)
		return 1;

	if (bind(sfd, (const SA *) addr, sizeof(*addr)) < 0) {
		debug_logerror("Failed to bind");
		return 1;
	}

	socklen_t addrlen = sizeof(*addr);
	if (getsockname(sfd, (SA *) addr, &addrlen) < 0) {
		return 1;
	}

	set_tranfer_address(sfd, addr);
	return 0;
}

static uint8_t create_rqdl(ClientRQ *clientrq, uint16_t id)
{
	clientrq->cl.header = ((header_t) (DOWNLOAD | id << CODERQ_BITSLEN));

	uint16_t feed_number;
	feed_number_manager(DOWNLOAD, &feed_number);

	char file_name[INPUT_SIZE];
	memset(file_name, 0, INPUT_SIZE);
	uint8_t datalen = data_manager(DOWNLOAD, file_name);

	SA_IN6 addr;
	transfer_new(file_name);
	create_transfer_address(&addr);

	clientrq->cl.feed_number = feed_number;
	clientrq->cl.count = ntohs(addr.sin6_port);
	clientrq->cl.datalen = datalen;
	strncpy(clientrq->cl.data, file_name, datalen);
	return 0;
}

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

uint8_t create_request(ClientRQ *clientrq, uint16_t id)
{
	coderq_t rq_type = clientrq->type;
	if (rq_type == REGISTRATION)
		return create_rqrg(clientrq);

	if (rq_type == DOWNLOAD)
		return create_rqdl(clientrq, id);

	uint16_t header = (uint16_t) (rq_type | (id << CODERQ_BITSLEN));
	uint16_t feed_number = 0;
	uint16_t count = 0;

	feed_number_manager(rq_type, &feed_number);
	count_number_manager(rq_type, &count);

	char input[INPUT_SIZE];
	memset(input, 0, INPUT_SIZE);
	uint8_t datalen = data_manager(rq_type, input);
	if (rq_type == UPLOAD) {
		file_manager(input);
		if (transfer_new(input))
			return 1;

		char *file_name = strrchr(input, '/');
		if (file_name != NULL) {
			size_t len = strlen(++file_name);
			memmove(input, file_name, len);
			input[len] = 0;
		}
	}

	clientrq->cl.header = header;
	clientrq->cl.feed_number = feed_number;
	clientrq->cl.count = count;
	clientrq->cl.datalen = datalen;

	memcpy(clientrq->cl.data, input, datalen);
	return 0;
}

uint8_t create_ftransfer_requests(Array *a_ftrq, size_t *f_size, uint16_t id)
{
	/* Récuperation du path complet du fichier enregistré
	 * lors du traitement de la requête TCP. */
	char *file_path = get_tranfer_file_path();

	int fd;
	if ((fd = open(file_path, O_RDONLY)) < 0) {
		perror("open");
		return 1;
	}

	struct stat st;
	if (fstat(fd, &st) < 0) {
		perror("stat");
		return 1;
	}

	*f_size = (size_t) st.st_size;

	char *file = NULL;
	file = mmap(NULL, *f_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (file == NULL) {
		perror("mmap");
		return 1;
	}

	uint16_t header = (uint16_t) (UPLOAD | (id << CODERQ_BITSLEN));
	*a_ftrq = ftransferrqs_new(header, file, st.st_size);
	if (a_ftrq->data == NULL)
		return 1;

	munmap(file, *f_size);
	close(fd);
	return 0;
}

int8_t handle_download_packet(ServerRQ *serverrq, size_t nbytes)
{
	if (serverrq->type != DOWNLOAD) {
		logerror("Invalid Packets");
		return 1;
	}

	uint16_t id = get_id(serverrq->ft.header);
	uint16_t numblock = serverrq->ft.numblock;
	if (get_user_id() != id) {
		logerror("Invalid Packets");
		return 1;
	}

	nbytes -= (sizeof(id) + sizeof(numblock));
	return add_packet(numblock, serverrq->ft.data, nbytes);
}

/* -------------------------------------------------------------------------- */
