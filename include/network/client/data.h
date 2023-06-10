#ifndef DATA_H
#define DATA_H


/* -------------------------------- INCLUDES -------------------------------- */

#include <stdint.h>
#include <stddef.h>

#include "network/network_macros.h"

#include "network/file_transfer.h"

#include "user/user.h"
#include "network/request.h"
#include "data_structures/queue.h"

/* -------------------------------- STRUCTURES ------------------------------ */

typedef struct
{
	uint16_t nbfeed;
	Queue messages;
} Notif;

typedef struct
{
	char data[NT_DATA_LEN];
	char pseudo[PSEUDO_LEN];
} Message;

typedef struct
{
	Mult mult_info;
	Notif *notif;
} Sb;

/* -------------------------------- FUNCTIONS ------------------------------- */

void data_init(void);
void data_free(void);

uint8_t transfer_new(const char *file_name);
void set_tranfer_address(int sfd, SA_IN6 *addr);
int get_transfer_socket(void);
void reset_transfer_socket(void);
char *get_tranfer_file_path(void);
int8_t add_packet(uint16_t numblock, char *data, size_t nbytes);
int subscription_add(const char *addr, uint16_t port, int sfd, uint16_t feed_nb);
int add_notif(int fd, char *message, char *pseudo);
size_t get_notification_count(void);
Notif *get_notif(size_t i);
size_t get_subscribe_count(void);
Sb *get_sb(size_t i);
void wait_notif(void);
uint8_t refresh_fds(Array *fds);
uint8_t check_transfer_timeout(void);
int wait_transfer(void);
void signal_transfer(void);
void clear_current_transfer(void);

/* -------------------------------------------------------------------------- */

#endif /* DATA_H */
