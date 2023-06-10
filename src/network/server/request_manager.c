/**
 * @file request_manager.c
 * @brief Impletation for handling requests from clients.
 */

#include "network/server/request_manager.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "network/server/data.h"
#include "network/request.h"
#include "system/logger.h"
#include <sys/mman.h>
/*---------------------------- PRIVATE FUNCTIONS --------------------------- */

/**
 * @brief Handles a registration request from a client
 *
 * @param clientrq The client request
 * @return A server request, or NULL if an error occured
 */
static uint8_t registration_request(Array *a_serverrq, ClientRQ *clientrq)
{
	if (get_nb_user() >= USER_MAX) {
		d_errno = ERR_IDMAX;
		return 1;
	}

	uint16_t id = generate_new_id();

	if (user_new(id, clientrq->rg.pseudo))
		return 1;

	header_t header = (header_t) (REGISTRATION | id << CODERQ_BITSLEN);

	ServerRQ serverrq;
	serverrq.cl.header = header;
	serverrq.cl.feed_number = 0;
	serverrq.cl.count = 0;

	if (a_serverrq->append(a_serverrq, &serverrq))
		return 1;

	return 0;
}

/**
 * @brief Handles a post request from a client
 *
 * @param clientrq The client request
 * @return A server request, or NULL if an error occured
 */
static uint8_t post_request(Array *a_serverrq, ClientRQ *clientrq)
{
	uint16_t feed_number = clientrq->cl.feed_number;
	size_t feed_count = get_feeds_count();
	/* Max feed number */
	if (feed_count >= FEED_NB_MAX && feed_number == 0) {
		d_errno = ERR_FEEDMAX;
		return 1;
	}
	/* feed number 'feed_number' doesn't exist */
	if (feed_number > feed_count) {
		d_errno = ERR_FEEDNB;
		return 1;
	}

	pseudo_t *pseudo = get_pseudo(get_id(clientrq->cl.header));
	/* ID doesn't exist */
	if (pseudo == NULL) {
		d_errno = ERR_NOID;
		return 1;
	}

	if (feed_number == 0) {
		/* new post on a new feed */
		feed_number = (uint16_t) feed_count + 1;
		if (feed_new(*pseudo))
			return 1;
	}

	char *data = clientrq->cl.data;
	if (post_new(*pseudo, clientrq->cl.datalen, data, feed_number))
		return 1;

	ServerRQ serverrq;
	serverrq.cl.header = clientrq->cl.header;
	serverrq.cl.feed_number = feed_number;
	serverrq.cl.count = clientrq->cl.count;

	if (a_serverrq->append(a_serverrq, &serverrq))
		return 1;

	return 0;
}

/**
 * @brief Handles a last posts request from a client
 *
 * @param clientrq The client request
 * @return A server request, or NULL if an error occured
 */
static uint8_t last_posts_request(Array *a_serverrq, ClientRQ *clientrq)
{
	uint16_t feed_number = clientrq->cl.feed_number;
	uint8_t all_feed = (feed_number == 0);
	uint16_t feed_count = all_feed ? (uint16_t) get_feeds_count() : 1;

	/* ID doesn't exist */
	if (get_pseudo(get_id(clientrq->cl.header)) == NULL) {
		d_errno = ERR_NOID;
		return 1;
	}

	/* thread number 'thread_number' doesn't exist */
	if (get_feeds_count() < feed_number) {
		d_errno = ERR_FEEDNB;
		return 1;
	}

	uint16_t count = clientrq->cl.count;
	size_t start_feed = all_feed ? 0 : feed_number - 1;
	a_serverrq->length = 1;

	for (size_t i = 0; i < feed_count; i++) {
		size_t i_feed = i + start_feed;

		Feed *feed = get_feeds(i_feed);
		Array *posts = &feed->posts;
		pseudo_t *creator = &feed->creator;

		/* nb of posts to treat */
		uint8_t all_post = (count == 0 || count > posts->length);
		size_t posts_count = all_post ? posts->length : count;

		size_t start_post = posts->length - posts_count;
		for (size_t j = 0; j < posts_count; j++) {
			size_t j_post = start_post + j;
			Post *post = posts->get(posts, j_post);

			ServerRQ serverrq;
			serverrq.lp = serverrq_lp_new(
				(uint16_t) (i_feed + 1), creator,
				&post->pseudo, post->datalen, post->data);

			if (a_serverrq->append(a_serverrq, &serverrq))
				return 1;
		}
	}

	ServerRQ *serverrq = a_serverrq->data;
	serverrq[0].cl.header = clientrq->cl.header;
	serverrq[0].cl.feed_number = all_feed ? feed_count : feed_number;
	serverrq[0].cl.count = (uint16_t) (a_serverrq->length - 1);
	return 0;
}

#define ADDR_MULT "ff12::1:2:3"
#include <arpa/inet.h>

static uint8_t set_mult(Mult *mult)
{
	int mult_sock;
	if((mult_sock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
		return 1;
	struct sockaddr_in6 grsock;
	memset(&grsock, 0, sizeof(grsock));
	grsock.sin6_family = AF_INET6;
	inet_pton(AF_INET6, ADDR_MULT, &grsock.sin6_addr);
	uint16_t port = 6226 + (uint16_t) get_subscribe_count() + 2;
	grsock.sin6_port = htons(port);
	int ifindex = 0;
	if(setsockopt(mult_sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex))) {
		perror("erreur initialisation de l interface locale");
		return 1;
	}
	memcpy(mult->addr, ADDR_MULT, 16);
	mult->port = port;
	mult->sock_fd = mult_sock;
	mult->sock_addr = grsock;
	return 0;
}

static uint8_t subscriptions_request(Array *a_serverrq, ClientRQ *clientrq)
{
	pseudo_t *pseudo = get_pseudo(get_id(clientrq->cl.header));
	/* ID doesn't exist */
	if (pseudo == NULL) {
		d_errno = ERR_NOID;
		return 1;
	}

	uint16_t feed_number = clientrq->cl.feed_number;
	size_t feed_count = get_feeds_count();
	/* feed number 'feed_number' doesn't exist */
	if (feed_number > feed_count || feed_number == 0) {
		d_errno = ERR_FEEDNB;
		return 1;
	}

	Feed *feed = get_feeds(feed_number);
	if (feed->notif == NULL) {
		NotificationsInfos notif;
		set_mult(&notif.mult_infos);
		notif_new(feed_number, notif.mult_infos.addr, notif.mult_infos.port, notif.mult_infos.sock_fd, notif.mult_infos.sock_addr);
	}

	ServerRQ serverrq;
	serverrq.sb.header = clientrq->cl.header;
	serverrq.sb.feed_number = feed_number;
	serverrq.sb.count = feed->notif->mult_infos.port;
	memcpy(serverrq.sb.addr, feed->notif->mult_infos.addr, 16);

	if (a_serverrq->append(a_serverrq, &serverrq))
		return 1;

	return 0;
}

static uint8_t prepare_upload_request(Array *a_serverrq, ClientRQ *clientrq)
{
	uint16_t feed_number = clientrq->cl.feed_number;
	size_t feed_count = get_feeds_count();
	pseudo_t *pseudo = get_pseudo(get_id(clientrq->cl.header));

	/* ID doesn't exist */
	if (pseudo == NULL) {
		d_errno = ERR_NOID;
		return 1;
	}

	/* feed number 'feed_number' doesn't exist */
	if (feed_number > feed_count) {
		d_errno = ERR_FEEDNB;
		return 1;
	}

	ServerRQ serverrq;
	serverrq.cl.header = clientrq->cl.header;
	serverrq.cl.feed_number = feed_number;
	serverrq.cl.count = UDP_PORT;

	if (a_serverrq->append(a_serverrq, &serverrq))
		return 1;

	uint16_t id = get_id(clientrq->cl.header);
	return transfer_new(id, feed_number, clientrq->cl.data);
}

static uint8_t prepare_download_request(Array *a_serverrq, ClientRQ *clientrq)
{
	uint16_t feed_number = clientrq->cl.feed_number;
	size_t feed_count = get_feeds_count();
	pseudo_t *pseudo = get_pseudo(get_id(clientrq->cl.header));

	/* ID doesn't exist */
	if (pseudo == NULL) {
		d_errno = ERR_NOID;
		return 1;
	}

	/* feed number 'feed_number' doesn't exist */
	if (feed_number > feed_count) {
		d_errno = ERR_FEEDNB;
		return 1;
	}
	/* file doesn't exist*/
	char file_path[MAX_DATALEN];
	strcpy(file_path, clientrq->cl.data);
	if (!file_exist(file_path, clientrq->cl.feed_number)) {
		d_errno = ERR_NOFILE;
		return 1;
	}

	ServerRQ serverrq;
	serverrq.cl.header = clientrq->cl.header;
	serverrq.cl.feed_number = feed_number;
	serverrq.cl.count = clientrq->cl.count;
	if (a_serverrq->append(a_serverrq, &serverrq))
		return 1;

	uint16_t id = get_id(clientrq->cl.header);
	return transfer_new(id, feed_number, file_path);
}

uint8_t create_ftransfer_requests(Array *a_ftrq, size_t *f_size, uint16_t id) {
	/* Récuperation du path complet du fichier enregistré
	 * lors du traitement de la requête TCP. */
	char *file_path = get_tranfer_file_path(id);

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

	uint16_t header = (uint16_t) (DOWNLOAD | (id << CODERQ_BITSLEN));
	*a_ftrq = ftransferrqs_new(header, file, st.st_size);
	if (a_ftrq->data == NULL)
		return 1;

	munmap(file, *f_size);
	close(fd);
	return 0;
}

/**
 * @brief An array of functions for handling each request type
 */
static uint8_t (*requests[]) (Array *a_serverrq, ClientRQ *clientrq) = {
	registration_request,
	post_request,
	last_posts_request,
	subscriptions_request,
	prepare_upload_request,
	prepare_download_request
};

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

uint8_t handle_tcp_request(Array *a_serverrq, ClientRQ *clientrq)
{
	d_errno = NOERROR;
	ServerRQ *serverrq = a_serverrq->data;
	if (requests[clientrq->type - 1](a_serverrq, clientrq) == 0) {
		d_errno = NOERROR;
		serverrq->type = clientrq->type;
		return 0;
	}

	/* error occured - returning error request */
	if (errno == ENOMEM)
		return 1;

	serverrq->cl = serverrq_error();
	serverrq->type = (coderq_t) d_errno;
	return 0;
}

uint8_t handle_upload_packet(ClientRQ *clientrq, size_t nbytes)
{
	if (clientrq->type != UPLOAD) {
		logerror("Invalid Packets");
		return 0;
	}

	uint16_t id = get_id(clientrq->ft.header);
	uint16_t numblock = clientrq->ft.numblock;
	nbytes -= (sizeof(id) + sizeof(numblock));
	return add_packet(id, numblock, clientrq->ft.data, nbytes);
}

/* -------------------------------------------------------------------------- */
