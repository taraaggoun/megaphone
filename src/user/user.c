#include "user/user.h"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "system/logger.h"


#define USER_DATALEN 	17
#define FILE_NAME 	"res/client/users.data"

static User             *accounts = NULL;
static struct flock     lock;

static int              fd = 0;
static size_t		fsize = 0;

static uint16_t 	m_id;

/* ---------------------------- PRIVATE FUNCTIONS --------------------------- */

static uint16_t get_nb_accounts(void)
{
	uint16_t nb_account = 0;
	while (nb_account < ID_MAX && accounts[nb_account].id != 0)
		nb_account++;

	return nb_account;
}

static void lock_rdwr(void)
{
	lock.l_type = F_WRLCK;
	fcntl(fd, F_SETLK, &lock);
	lock.l_type = F_RDLCK;
	fcntl(fd, F_SETLK, &lock);
}

static void unlock_rdwr(void)
{
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
}

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

uint16_t get_user_id(void)
{
	return m_id;
}

void set_user_id(uint16_t id)
{
	m_id = id;
}

User user_init(uint16_t id, pseudo_t pseudo)
{
	User user;

	user.id = id;
	memcpy(user.pseudo, pseudo, PSEUDO_LEN);

	return user;
}

void unload_accounts(void)
{
	munmap(accounts, fsize);
	close(fd);
}

uint8_t load_accounts(void)
{
	fd = open(FILE_NAME, O_RDWR | O_EXCL);
	if (fd == -1 && errno == ENOENT) {
		/* Le fichier n'existe pas */
		fd = open(FILE_NAME, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
		if (fd < 0) {
			perror("open");
			return 1;
		}

		off_t file_len = sizeof(User) * ID_MAX;
		if (ftruncate(fd, file_len) < 0) {
			perror("ftruncate");
			return 1;
		}
	}

	struct stat st;
	if (stat(FILE_NAME, &st) < 0) {
		perror("stat");
		return 1;
	}

	fsize = (size_t) st.st_size;
	int prot = PROT_READ | PROT_WRITE;
	accounts = mmap(NULL, fsize, prot, MAP_SHARED, fd, 0);
	if (accounts == MAP_FAILED)
		return 1;

	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;

	atexit(unload_accounts);
	return 0;
}

uint8_t add_account(uint16_t id, pseudo_t pseudo)
{
	lock_rdwr();

	uint16_t nb_users = get_nb_accounts();
	if (nb_users == ID_MAX) {
		unlock_rdwr();
		return 1;
	}

	accounts[nb_users] = user_init(id, pseudo);
    	unlock_rdwr();

	return 0;
}

uint8_t show_accounts(void)
{
	lock_rdwr();

	uint16_t nb_users = get_nb_accounts();
	if (nb_users == 0)
		return 1;

	log_msg(CYAN, "%d account find:\n", nb_users);
	for (uint16_t i = 0; i < nb_users; i++) {
		char buf[USER_DATALEN];
		sprintf(buf, "%.4d: ", accounts[i].id);
		strncat(buf, accounts[i].pseudo, PSEUDO_LEN);

		char *new_line = strchr(buf, '#');
		if (new_line != NULL)
			*new_line = '\0';

		log_msg(CYAN, "\t%s\n", buf);
	}

	unlock_rdwr();
	return 0;
}

uint8_t id_exist(uint16_t id)
{
	lock_rdwr();

	uint8_t found = 0;
	uint16_t nb_users = get_nb_accounts();
	for (int i = 0; i < nb_users; i++) {
		if (accounts[i].id == id) {
			found = 1;
			break;
		}
	}

	unlock_rdwr();
	return found;
}

/* -------------------------------------------------------------------------- */
