#include "network/client/data.h"

#include <poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>

#if defined(__APPLE__)
#include <limits.h>
#else
#include <linux/limits.h>
#endif

#include "network/request.h"


static FileTransferInfos m_tranfer_file;
static pthread_mutex_t   m_transfer_mutex;
static pthread_cond_t    m_transfer_cond;

static Array 		 m_subscriptions;
static pthread_mutex_t   m_subscriptions_mutex;
static pthread_cond_t    m_subscriptions_cond;

static Array 		 m_notification;
static pthread_mutex_t   m_notification_mutex;


/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

static void lock_transfer(void)
{
	pthread_mutex_lock(&m_transfer_mutex);
}

static void unlock_transfer(void)
{
	pthread_mutex_unlock(&m_transfer_mutex);
}

static void lock_subscriptions(void)
{
	pthread_mutex_lock(&m_subscriptions_mutex);
}

static void unlock_subscriptions(void)
{
	pthread_mutex_unlock(&m_subscriptions_mutex);
}

static void lock_notification(void)
{
	pthread_mutex_lock(&m_notification_mutex);
}

static void unlock_notification(void)
{
	pthread_mutex_unlock(&m_notification_mutex);
}

void data_init(void)
{
	memset(&m_tranfer_file, 0, sizeof(m_tranfer_file));
	pthread_mutex_init(&m_transfer_mutex, NULL);
	pthread_cond_init(&m_transfer_cond, NULL);

	if (array_new(&m_subscriptions, sizeof(Sb), 0))
		exit(EXIT_FAILURE);

	pthread_mutex_init(&m_subscriptions_mutex, NULL);
	pthread_cond_init(&m_subscriptions_cond, NULL);

	if (array_new(&m_notification, sizeof(Notif), 0))
		exit(EXIT_FAILURE);

	pthread_mutex_init(&m_notification_mutex, NULL);
}

void data_free(void)
{
	if (m_tranfer_file.active)
		array_free(&m_tranfer_file.file_data);

	pthread_mutex_destroy(&m_transfer_mutex);
	pthread_cond_destroy(&m_transfer_cond);

	array_free(&m_subscriptions);
	pthread_mutex_destroy(&m_subscriptions_mutex);
	pthread_cond_destroy(&m_subscriptions_cond);

	array_free(&m_notification);
	pthread_mutex_destroy(&m_notification_mutex);
}

void wait_notif(void)
{
    lock_subscriptions();
    while(m_subscriptions.length == 0) {
        pthread_cond_wait(&m_subscriptions_cond,
                  &m_subscriptions_mutex);
    }
    unlock_subscriptions();
}

uint8_t transfer_new(const char *file_name)
{
	uint8_t err;

	lock_transfer();
	if (m_tranfer_file.active)
		transfer_clear(&m_tranfer_file);

	m_tranfer_file = transfer_init(file_name, 0);
	err = (m_tranfer_file.file_data.data == NULL);
	unlock_transfer();

	return err;
}

void set_tranfer_address(int sfd, SA_IN6 *addr)
{
	lock_transfer();
	if (!m_tranfer_file.active) {
		unlock_transfer();
		return;
	}

	m_tranfer_file.sfd = sfd;
	m_tranfer_file.addr = *addr;
	unlock_transfer();
}

int get_transfer_socket(void)
{
	lock_transfer();
	int sfd = m_tranfer_file.sfd;
	unlock_transfer();

	return sfd;
}

void reset_transfer_socket(void)
{
	lock_transfer();
	m_tranfer_file.sfd = 0;
	unlock_transfer();
}

char *get_tranfer_file_path(void)
{
	lock_transfer();
	char *file_path = m_tranfer_file.file_path;
	unlock_transfer();

	return file_path;
}

int subscription_add(const char *addr, uint16_t port, int sfd, uint16_t feed_nb)
{
	Sb sb;
	Mult mult;
	Notif notif;

	notif.nbfeed = feed_nb;
	notif.messages = queue_init(sizeof(Message));

	lock_notification();
	if (m_notification.append(&m_notification, &notif)) {
		unlock_notification();
		return 1;
	}
	sb.notif = m_notification.get(&m_notification, m_notification.length - 1);
	unlock_notification();
	
	memcpy(mult.addr, addr, sizeof(*addr));
	mult.port = port;
	mult.sock_fd = sfd;
	sb.mult_info = mult;

	int err;
	lock_subscriptions();
	err = m_subscriptions.append(&m_subscriptions, &sb);

	if (m_subscriptions.length == 1)
		pthread_cond_signal(&m_subscriptions_cond);

	unlock_subscriptions();

	return err;
}

int add_notif(int fd, char *message, char *pseudo)
{
	lock_subscriptions();

	for (size_t i = 0; i < m_subscriptions.length; i++) {
		Sb *sb = m_subscriptions.get(&m_subscriptions, i);
		if (sb->mult_info.sock_fd != fd)
			continue;

		int err;
		Message msg;
		memcpy(msg.data, message, NT_DATA_LEN);
		memcpy(msg.pseudo, pseudo, PSEUDO_LEN);
		err = sb->notif->messages.enqueue(&sb->notif->messages, &msg);
		unlock_subscriptions();

		return err;
	}

	unlock_subscriptions();
	return 1;
}

size_t get_notification_count(void)
{
	lock_notification();
	size_t length = m_notification.length;
	unlock_notification();

	return length;
}

size_t get_subscribe_count(void)
{
	lock_subscriptions();
	size_t length = m_subscriptions.length;
	unlock_subscriptions();

	return length;
}

Notif *get_notif(size_t i)
{
	lock_notification();
	Notif *n = m_notification.get(&m_notification, i);
	unlock_notification();

	return n;
}

Sb *get_sb(size_t i)
{
	lock_subscriptions();
	Sb *s = m_subscriptions.get(&m_subscriptions, i);
	unlock_subscriptions();

	return s;
}

uint8_t refresh_fds(Array *fds)
{
    lock_subscriptions();
    size_t subs_count = m_subscriptions.length;
    if (fds->length == subs_count) {
        unlock_subscriptions();
        return 1;
    }

    Sb *subs = m_subscriptions.data;
    for (size_t i = fds->length; i < subs_count; i++) {
        int sfd = subs[i].mult_info.sock_fd;
        struct pollfd pfd = { .fd = sfd, .events = POLLIN};
        if (fds->append(fds, &pfd)) {
		unlock_subscriptions();
            	return 1;
	}
    }

    unlock_subscriptions();
    return 0;
}

static uint8_t write_file(FileTransferInfos *infos)
{
	char *home = getenv("HOME");
	if (home == NULL)
		return 1;

	char dl_file_path[PATH_MAX];
	memset(dl_file_path, 0, PATH_MAX);
	sprintf(dl_file_path, "%s/Téléchargements", home);

	char file_path[MAX_DATALEN];
	memset(file_path, 0, MAX_DATALEN);
	if (snprintf(file_path, MAX_DATALEN, "%s/%s", dl_file_path,
		     infos->file_path) < 0)
		return 1;

	int fd = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		memset(dl_file_path, 0, PATH_MAX);
		sprintf(dl_file_path, "%s/Downloads", home);
		
		memset(file_path, 0, MAX_DATALEN);
		if (snprintf(file_path, MAX_DATALEN, "%s/%s", dl_file_path,
		     infos->file_path) < 0)
			return 1;
		fd = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if (fd < 0) {
			mkdir(dl_file_path, S_IRWXU);
			fd = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			if (fd < 0) {
				perror("open");
				return 1;
			}
		}

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

int8_t add_packet(uint16_t numblock, char *data, size_t nbytes)
{
	lock_transfer();
	FileTransferInfos *infos = &m_tranfer_file;

	/* Pas de demande de Download au préalable, paquet non traité. */
	if (!infos->active) {
		unlock_transfer();
		return 1;
	}

	/*
	 * Dernier numero de paquet, enregistrement pour savoir quand est
	 * ce qu'on a tous les paquets.
	 */
	if (nbytes < FILE_PACKET_SIZE)
		infos->last_packet = numblock;

	if (infos->file_data.set(&infos->file_data, numblock - 1, data)) {
		unlock_transfer();
		return -1;
	}

	infos->file_size += (off_t) nbytes;

	/* Tous les paquets sont recus, ecriture du fichier sur le disque. */
	if (infos->file_data.length == infos->last_packet) {
		if (write_file(infos)) {
			unlock_transfer();
			return -1;
		}

		close(infos->sfd);
		transfer_clear(infos);
		unlock_transfer();
		return 0;
	}

	unlock_transfer();
	return 1;
}

uint8_t check_transfer_timeout(void)
{
	lock_transfer();

	time_t now = time(NULL);
	uint8_t timeout = difftime(now, m_tranfer_file.begin) > FT_TIMEOUT_SEC;

	if (m_tranfer_file.active && timeout) {
		close(m_tranfer_file.sfd);
		transfer_clear(&m_tranfer_file);

		unlock_transfer();
		return 1;
	}

	unlock_transfer();
	return 0;
}

void clear_current_transfer(void)
{
	lock_transfer();
	transfer_clear(&m_tranfer_file);
	unlock_transfer();
}

int wait_transfer(void)
{
	lock_transfer();
	while (m_tranfer_file.sfd == 0)
		pthread_cond_wait(&m_transfer_cond, &m_transfer_mutex);

	int sfd = m_tranfer_file.sfd;
	unlock_transfer();

	return sfd;
}

void signal_transfer(void)
{
	pthread_cond_signal(&m_transfer_cond);
}

/* -------------------------------------------------------------------------- */
