#include "network/client/tcp_client.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "user/user.h"
#include "user/user_input.h"

#include "network/request.h"
#include "network/client/data.h"
#include "network/client/client.h"
#include "network/client/network.h"
#include "network/client/request_manager.h"

#include "system/logger.h"

static char m_port[PORT_STRLEN];


/* ---------------------------- PRIVATE FUNCTIONS --------------------------- */

/* ------------- AFFICHAGE ------------- */

static void display_actions(void)
{
	log_msg(BLUE, "Here is the list of possible actions:");
	log_msg(BLUE, "\t1: New post");
	log_msg(BLUE, "\t2: Get last post");
	log_msg(BLUE, "\t3: Subscribe to a feed");
	log_msg(BLUE, "\t4: Upload a file");
	log_msg(BLUE, "\t5: Download a file");
	log_msg(BLUE, "\t6: Notification");
	log_msg(BLUE, "\t7: Log out");
	log_msg(BLUE, "\t8: Exit\n");
}

static void display_authentication_actions(void)
{
	log_msg(BLUE, "Here is the list of possible actions:");
	log_msg(BLUE, "\t1: Sign up");
	log_msg(BLUE, "\t2: Log in");
	log_msg(BLUE, "\t3: Exit\n");
}

static void get_pseudo(char *dst, const char *pseudo)
{
	memset(dst, 0, PSEUDO_LEN);
	for (int i = 0; i < PSEUDO_LEN; i++) {
		if (pseudo[i] == '#')
			break;

		dst[i] = pseudo[i];
	}
}

static void get_data(char *dst, const char *data, uint8_t datalen)
{
	memset(dst, 0, datalen);
	int id_data = 0;
	int id = 0;
	while (id_data < datalen) {
		dst[id] = data[id_data];
		id_data++;
		id++;

		if ((id + 1) % 35 == 0) {
            		dst[id] = '\n';
			dst[id + 1] = ' ';
			id += 2;
		}
	}
	dst[datalen - 1] = '\0';
}

static void print_last_post(ServerRQ *s_rq)
{
	size_t count = (size_t) s_rq->cl.count;
	size_t feed_nb = 0;
	char pseudo[PSEUDO_LEN];
	for (size_t i = 0; i < count; i++) {
		size_t c_feed_nb = (size_t) s_rq[i + 1].lp.feed_number;
		if (feed_nb != c_feed_nb) {
			log_msg(CYAN, "############### FEED %d ###############\n", c_feed_nb);
			feed_nb = c_feed_nb;
			get_pseudo(pseudo, s_rq[i + 1].lp.creator);
			log_msg(BLUE, " creator of feed : %s", pseudo);
		}
		log_msg(CYAN, "\n################ POST ################\n");
		get_pseudo(pseudo, s_rq[i + 1].lp.pseudo);
		log_msg(BLUE, " post by %s:", pseudo);
		int len = s_rq[i + 1].lp.datalen + (int)(s_rq[i + 1].lp.datalen / 10) + 1;
		char data[len];
		get_data(data, s_rq[i + 1].lp.data, (uint8_t)len);
		log_msg(BLUE, "\n %s\n", data);
	}
	log_msg(CYAN, "######################################\n");
}

static void print_server_answer(coderq_t type, ServerRQ *serverrq)
{
	if (type == REGISTRATION)
		logsuccess("Registration success: your ID is %.4u\n",
			get_id(serverrq->cl.header));

	if (type == NEWPOST)
		logsuccess("Your post successfully post on feed %d\n",
			serverrq->cl.feed_number);

	if (type == LASTPOSTS)
		print_last_post(serverrq);

	if (type == SUBSCRIBE)
		logsuccess("Subscription to feed %d success\n", serverrq->sb.feed_number);

	if (d_errno != NOERROR)
		logerror("%s\n", strerrno());
}

static void print_notif(void)
{
	size_t nb_feed = get_notification_count();
	int have_notif = 0;
	for (size_t i = 0; i < nb_feed; i++) {
		Notif *notif = get_notif(i);
		if (notif->messages.length > 0) {
			have_notif = 1;
			log_msg(CYAN, "############### FEED %d ###############\n", notif->nbfeed);
			Node *message = notif->messages.dequeue(&notif->messages);
			while(message != NULL) {
				log_msg(CYAN, "\n################ POST ################\n");
				char pseudo[PSEUDO_LEN];
				Message *msg = message->data;
				get_pseudo(pseudo, msg->pseudo);
				log_msg(BLUE, " post by %s:", pseudo);
				char data[NT_DATA_LEN];
				get_data(data, msg->data, (uint8_t)NT_DATA_LEN);
				log_msg(BLUE, "\n %s\n", data);
				node_free(message);
				message = NULL;
				message = notif->messages.dequeue(&notif->messages);
			}

			log_msg(CYAN, "######################################\n");
		}
	}
	if (!have_notif)
		logerror("You have 0 notification on 0 feed \n");
}

/* --------------------------------------------- */

static uint8_t handle_file_upload(const char *port)
{
	Array a_ftrq;
	size_t file_size = 0;
	if (create_ftransfer_requests(&a_ftrq, &file_size, get_user_id()))
		return 1;

	/* Ici le tableau 'a_ftrq' contient tous les paquets à envoyer. */
	Client udpclient = client_new(port, DOMAIN, SOCK_DGRAM);
	if (connect_client(&udpclient)) {
		array_free(&a_ftrq);
		return 1;
	}

	if (send_ftransfer_requests(&udpclient, &a_ftrq, file_size)) {
		close(udpclient.sfd);
		clear_current_transfer();
		array_free(&a_ftrq);
		return 1;
	}

	close(udpclient.sfd);
	clear_current_transfer();
	array_free(&a_ftrq);
	return 0;
}

static uint8_t client_callback(ClientRQ *clientrq, ServerRQ *serverrq)
{
	coderq_t type = serverrq->type;
	if (type == REGISTRATION) {
		set_user_id(get_id(serverrq->cl.header));
		if (add_account(get_user_id(), clientrq->rg.pseudo))
			return 1;
	}

	if (type == UPLOAD) {
		char port[16];
		memset(port, 0, 16);
		snprintf(port, 16, "%u", serverrq->cl.count);
		if (handle_file_upload(port))
			return 1;
	}

	if (type == SUBSCRIBE) {
		int mult_sock;
		if ((mult_sock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
			return 1;
		SA_IN6 grsock;
		memset(&grsock, 0, sizeof(grsock));
		grsock.sin6_family = AF_INET6;
		grsock.sin6_addr = in6addr_any;
		grsock.sin6_port = htons(serverrq->sb.count);
		int ok = 1;
		if (setsockopt(mult_sock, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok)) < 0) {
			perror("echec de SO_REUSEADDR");
			close(mult_sock);
			return 1;
		}
		if (bind(mult_sock, (SA *) &grsock, sizeof(grsock))) {
			perror("erreur bind");
			close(mult_sock);
		}
		struct ipv6_mreq group;
		inet_pton(AF_INET6, serverrq->sb.addr, &group.ipv6mr_multiaddr.s6_addr);
		group.ipv6mr_interface = 0;
		if (setsockopt(mult_sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group, sizeof(group)) < 0) {
			perror("erreur abonnement groupe");
			close(mult_sock);
		}
		subscription_add(serverrq->sb.addr, ntohs(serverrq->sb.count), mult_sock, serverrq->sb.feed_number);
	}

	if (type == DOWNLOAD) {
		signal_transfer();
	}

	return 0;
}

static uint8_t handle_client_connection(coderq_t type)
{
	d_errno = NOERROR;
	/* create client request */
	ClientRQ clientrq;
	memset(&clientrq, 0, sizeof(clientrq));

	clientrq.type = type;
	if (create_request(&clientrq, get_user_id())) {
		logerror("create_request");
		return 1;
	}

	debug_clientrq(&clientrq);

	/* connect client */
	Client tcpclient = client_new(m_port, DOMAIN, SOCK_STREAM);
	if (connect_client(&tcpclient)) {
		logerror("connect_client");
		return 1;
	}

	/* send client request */
	if (send_client_request(&tcpclient, &clientrq)) {
		logerror("send_client_request");
		close(tcpclient.sfd);
		return 1;
	}

	/* receive server request */
	ServerRQ *serverrq = NULL;
	if (recv_server_request(tcpclient.sfd, &serverrq)) {
		logerror("recv_server_request\n");
		close(tcpclient.sfd);
		return 1;
	}

	debug_serverrq(serverrq);

	/* Give back information to the user */
	/* And process the server respond */
	print_server_answer(serverrq->type, serverrq);
	if (client_callback(&clientrq, serverrq)) {
		perror("client_callback");
		return 1;
	}

	free(serverrq);
	close(tcpclient.sfd);
	return 0;
}

static uint16_t select_account(void)
{
	uint16_t account;

	printf("\n");
	account = (uint16_t) readint("Which account do you want to use ?");
	if (!id_exist(account)) {
		logerror("the account doesn't exist\n");
		return 0;
	}

	logsuccess("Connection success: your ID is %.4d\n\n", account);
	return account;
}

static uint8_t authenticate(void)
{
	int action;

	while (1) {
		display_authentication_actions();
		action = readint("Enter the action you want to perform:");
		if (action < 0 || action > 3) {
			logerror("This action does not exist\n\n");
			continue;
		}
		break;
	}
	/* action is valid */
	switch (action) {
	case 1:
		return handle_client_connection(REGISTRATION);

	case 2:
		if (show_accounts()) {
			logerror("No account find! Please register\n");
			set_user_id(0);
			break;
		}
		set_user_id(select_account());
		break;

	default:
		exit(EXIT_SUCCESS);
	}
	return 0;
}

static uint8_t client_action(void)
{
	int action;
	while (1) {
		display_actions();
		action = readint("Enter the action you want to perform:");
		if (action < 1 || action > 8) {
			logerror("This action does not exist\n\n");
			continue;
		}
		if (action > 6)
			break;

		if (action == 6) {
			print_notif();
			continue;
		}

		if (handle_client_connection((coderq_t) (action + 1)))
			return 1;
	}
	/* action is valid */
	switch (action) {
	case 7:
		return 0;

	case 8:
		exit(EXIT_SUCCESS);

	default:
		exit(EXIT_FAILURE);
	}
}

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

uint8_t tcp_client_init(const char *port)
{
	strncpy(m_port, port, PORT_STRLEN);

	return 0;
}

void *tcp_client_loop(__attribute__((unused)) void *arg)
{
	log_msg(PURPLE, "\t\tWelcome on Megaphone\n");

	while (1) {
		if (authenticate())
			break;

		if (get_user_id() == 0)
			continue;

		if (client_action())
			break;
	}

	return NULL;
}

/* -------------------------------------------------------------------------- */
