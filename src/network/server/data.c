/**
 * @file data.c
 * @brief Contains functions to manage the data for the server.
 */

#include "network/server/data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>

#include "data_structures/array.h"
#include "network/file_transfer.h"


/**
 * @brief Array of all registered users.
 */
static Array m_known_users;
static pthread_mutex_t m_users_mutex;

/**
 * @brief Array of all feeds.
 */
static Array m_feeds;
static pthread_mutex_t m_feeds_mutex;

static Array m_tranfer_files;
static pthread_mutex_t m_tranfer_mutex;

static Array m_suscribe;

/* ---------------------------- PRIVATE FUNCTIONS --------------------------- */

static void lock_users(void)
{
	pthread_mutex_lock(&m_users_mutex);
}

static void unlock_users(void)
{
	pthread_mutex_unlock(&m_users_mutex);
}

static void lock_feeds(void)
{
	pthread_mutex_lock(&m_feeds_mutex);
}

static void unlock_feeds(void)
{
	pthread_mutex_unlock(&m_feeds_mutex);
}

static void lock_transfer(void)
{
	pthread_mutex_lock(&m_tranfer_mutex);
}

static void unlock_transfer(void)
{
	pthread_mutex_unlock(&m_tranfer_mutex);
}


static int8_t exist_id(uint16_t id)
{
	int8_t found = 0;
	if (id == 0)
		return 1;

	lock_users();
	User *users = m_known_users.data;
	for (size_t i = 0; i < m_known_users.length; i++) {
		if (users[i].id == id) {
			found = 1;
			break;
		}
	}
	unlock_users();

	return found;
}


/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

void data_init(void)
{
	atexit(data_free);

	pthread_mutex_init(&m_users_mutex, NULL);
	if (array_new(&m_known_users, sizeof(User), 0))
		exit(EXIT_FAILURE);

	pthread_mutex_init(&m_feeds_mutex, NULL);
	if (array_new(&m_feeds, sizeof(Feed), 0))
		exit(EXIT_FAILURE);

	pthread_mutex_init(&m_tranfer_mutex, NULL);
	if (array_new(&m_tranfer_files, sizeof(FileTransferInfos), ID_MAX + 1))
		exit(EXIT_FAILURE);

	if (array_new(&m_suscribe, sizeof(NotificationsInfos), 0))
		exit(EXIT_FAILURE);

	if (mkdir(UPLOAD_FILES_PATH, S_IRWXU) < 0 && errno != EEXIST)
		exit(EXIT_FAILURE);

	srandom((unsigned int) time(NULL));
}


void data_free(void)
{
	array_free(&m_known_users);
	pthread_mutex_destroy(&m_users_mutex);

	Feed *feeds = m_feeds.data;
	for (size_t i = 0; i < m_feeds.length; i++)
		array_free(&feeds[i].posts);

	array_free(&m_feeds);
	pthread_mutex_destroy(&m_feeds_mutex);

	FileTransferInfos *infos = m_tranfer_files.data;
	for (size_t i = 0; i < m_tranfer_files.length; i++)
		array_free(&infos[i].file_data);

	array_free(&m_tranfer_files);
	pthread_mutex_destroy(&m_tranfer_mutex);

	array_free(&m_suscribe);
}

/* ----------- Users ------------- */

int user_new(uint16_t id, pseudo_t pseudo)
{
	User user = user_init(id, pseudo);

	lock_users();
	int err = m_known_users.append(&m_known_users, &user);
	unlock_users();

	return err;
}

size_t get_nb_user(void)
{
	lock_users();
	size_t len = m_known_users.length;
	unlock_users();

	return len;
}

pseudo_t *get_pseudo(uint16_t id)
{
	lock_users();
	for (size_t i = 0; i < m_known_users.length; i++) {
		User *user = m_known_users.get(&m_known_users, i);
		if (user->id == id) {
			unlock_users();
			return &user->pseudo;
		}
	}
	unlock_users();

	return NULL;
}

uint16_t generate_new_id(void)
{
	uint16_t id;
	do {
		id = (uint16_t) random() % ID_MAX + 1;
	} while (exist_id(id));

	return id;
}

/* ------------------------------- */

/* ----------- Feeds ------------- */

int post_new(pseudo_t pseudo, uint8_t datalen,
	     char *data, size_t feed_number)
{
	Post post;
	int err;

	memcpy(post.pseudo, pseudo, PSEUDO_LEN);
	memcpy(post.data, data, datalen);
	post.datalen = datalen;

	/* feed indices = feed number - 1 */
	lock_feeds();
	Feed *feed = m_feeds.get(&m_feeds, feed_number - 1);
	err = feed->posts.append(&feed->posts, &post);
	unlock_feeds();

	return err;
}

uint8_t feed_new(pseudo_t creator)
{
	Feed new_feed;

	memcpy(new_feed.creator, creator, PSEUDO_LEN);
	if (array_new(&new_feed.posts, sizeof(Post), 0))
		return 1;

	lock_feeds();
	if (m_feeds.append(&m_feeds, &new_feed)) {
		unlock_feeds();
		return 1;
	}
	unlock_feeds();

	new_feed.notif = NULL;

	char feed_path[MAX_DATALEN];
	memset(feed_path, 0, MAX_DATALEN);
	snprintf(feed_path, MAX_DATALEN, "%s/%zu", UPLOAD_FILES_PATH,
		 m_feeds.length);

	struct stat st;
	if (stat(feed_path, &st) == -1) {
		if (mkdir(feed_path, S_IRWXU) < 0)
			return 1;

		return 0;
	}

	return 0;
}

uint8_t notif_new(size_t nbfeed, char *addr, uint16_t port, int fd, SA_IN6 sock_addr)
{
	NotificationsInfos notif;
	Mult mult;

	mult.port = port;
	mult.sock_fd = fd;
	memcpy(mult.addr, addr, 16);

	notif.nbfeed = nbfeed;
	notif.last_send_post = 0;
	notif.mult_infos = mult;
	notif.mult_infos.sock_addr = sock_addr;

	if (m_suscribe.append(&m_suscribe, &notif))
		return 1;

	Feed *feed = get_feeds(nbfeed);
	feed->notif = m_suscribe.get(&m_suscribe, m_suscribe.length - 1);

	return 0;
}

Feed *get_feeds(size_t feed_number)
{
	lock_feeds();
	Feed *feed = m_feeds.get(&m_feeds, feed_number);
	unlock_feeds();

	return feed;
}
NotificationsInfos *get_notif_info(size_t index)
{
	return m_suscribe.get(&m_suscribe, index);
}

NotificationsInfos *get_feed_notif_info(uint16_t nbfeed)
{
	for (size_t i = 0; i < m_suscribe.length; i++) {
		NotificationsInfos *infos = m_suscribe.get(&m_suscribe, i);
		if (infos->nbfeed == nbfeed)
			return infos;
	}
	return NULL;
}


size_t get_feeds_count(void)
{
	lock_feeds();
	size_t len = m_feeds.length;
	unlock_feeds();

	return len;
}

size_t get_subscribe_count(void)
{
	return m_suscribe.length;
}

/* ------------------------------- */

/* ---------- Transfers ---------- */

uint8_t transfer_new(uint16_t id, uint16_t feed_number, const char *file_name)
{
	uint8_t err;

	lock_transfer();
	FileTransferInfos *infos = m_tranfer_files.get(&m_tranfer_files, id);
	if (infos != NULL && infos->active)
		transfer_clear(infos);

	FileTransferInfos new_infos = transfer_init(file_name, feed_number);
	err = (uint8_t) m_tranfer_files.set(&m_tranfer_files, id, &new_infos);
	unlock_transfer();

	return err;
}

char *get_tranfer_file_path(uint16_t id)
{
	lock_transfer();
	FileTransferInfos *infos = m_tranfer_files.get(&m_tranfer_files, id);
	unlock_transfer();

	return infos->file_path;
}

void id_clear_transfer(uint16_t id)
{
	lock_transfer();
	FileTransferInfos *infos = m_tranfer_files.get(&m_tranfer_files, id);
	transfer_clear(infos);
	unlock_transfer();
}

void check_transfers_timeout(void)
{
	uint8_t timeout;
	time_t now = time(NULL);
	FileTransferInfos *tfs = m_tranfer_files.data;

	for (size_t i = 0; i < m_tranfer_files.length; i++) {
		timeout = difftime(now, tfs[i].begin) > FT_TIMEOUT_SEC;

		if (tfs[i].active && timeout) {
			close(tfs[i].sfd);
			transfer_clear(tfs + i);
		}
	}
}

static uint8_t write_file(FileTransferInfos *infos)
{
	char file_path[MAX_DATALEN];
	memset(file_path, 0, MAX_DATALEN);
	if (snprintf(file_path, MAX_DATALEN, "%s/%u/%s", UPLOAD_FILES_PATH,
		     infos->feed_number, infos->file_path) < 0)
		return 1;

	int fd = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	if (ftruncate(fd, infos->file_size) < 0) {
		perror("ftruncate");
		return 1;
	}

	int prot = PROT_READ | PROT_WRITE;
	char *file = mmap(NULL, (size_t) infos->file_size, prot, MAP_SHARED, fd, 0);
	if (file == NULL) {
		perror("mmap");
		return 1;
	}

	char *file_content = infos->file_data.data;
	memcpy(file, file_content, (size_t) infos->file_size);
	munmap(file, (size_t) infos->file_size);
	close(fd);
	return 0;
}

uint8_t add_packet(uint16_t id, uint16_t numblock, char *data, size_t nbytes)
{
	lock_transfer();
	FileTransferInfos *infos = m_tranfer_files.get(&m_tranfer_files, id);
	if (infos == NULL) {
		unlock_transfer();
		return 1;
	}

	/* Pas de demande de Upload au préalable, paquet non traité. */
	if (!infos->active) {
		unlock_transfer();
		return 0;
	}

	/*
	 * Dernier numero de paquet, enregistrement pour savoir quand est
	 * ce qu'on a tous les paquets.
	 */
	if (nbytes < FILE_PACKET_SIZE)
		infos->last_packet = numblock;

	if (infos->file_data.set(&infos->file_data, numblock - 1, data)) {
		unlock_transfer();
		return 1;
	}
	infos->file_size += (off_t) nbytes;

	/* Tous les paquets sont recus, ecriture du fichier sur le disque. */
	if (infos->file_data.length == infos->last_packet) {
		pseudo_t *pseudo = get_pseudo(id);
		if (infos->feed_number == 0 && feed_new(*pseudo)) {
			unlock_transfer();
			return 1;
		}

		if (infos->feed_number == 0)
			infos->feed_number = (uint16_t) (m_feeds.length);

		char file_name[MAX_DATALEN];
		memset(file_name, 0, MAX_DATALEN);

		if (snprintf(file_name, MAX_DATALEN, "%s %ld", infos->file_path, infos->file_size) < 0) {
			unlock_transfer();
			return 1;
		}

		if (post_new(*pseudo, (uint8_t) strlen(file_name),
			     file_name, infos->feed_number)) {
			unlock_transfer();
			return 1;
		}

		if (write_file(infos)) {
			unlock_transfer();
			return 1;
		}

		transfer_clear(infos);
	}

	unlock_transfer();
	return 0;
}

/* ------------------------------- */


/* -------------------------------------------------------------------------- */
