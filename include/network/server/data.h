#ifndef DATA_H
#define DATA_H

/* -------------------------------- INCLUDES -------------------------------- */

#include "user/user.h"

#include "network/network_macros.h"
#include "network/file_transfer.h"
#include "network/request.h"

#define USER_MAX     2047
#define FEED_NB_MAX  65536

/* -------------------------------- STRUCTURES ------------------------------ */

typedef struct
{
	pseudo_t pseudo;

	uint8_t datalen;
	char data[MAX_DATALEN];
} Post;

typedef struct
{
	Array posts;
	pseudo_t creator;

	NotificationsInfos *notif;
} Feed;

/* -------------------------------- FUNCTIONS ------------------------------- */

/**
 * @brief Initializes the data arrays for the chat server.
 *
 * This function initializes the global arrays known_users and feeds and
 * registers the data_free() function to be called on exit.
 */
void data_init(void);

/**
 * @brief Frees the data arrays for the chat server.
 *
 * This function frees the memory allocated for the global 
 * arrays known_users and feeds and their contents.
 */
void data_free(void);

/* --------------------------------------------- */

/**
 * @brief Creates a new user with the specified ID and pseudo.
 *
 * This function creates a new user with the specified ID and pseudo,
 * initializes its arrays and adds it to the known_users array.
 *
 * @param id The ID of the user
 * @param pseudo The pseudo of the user
 * @return 0 if success, or 1 if the append failed.
 */
int user_new(uint16_t id, pseudo_t pseudo);

uint16_t generate_new_id(void);

/**
 * @brief Gets the pseudo of a user with the specified ID.
 *
 * This function searches the known_users array for a user with 
 * the specified ID and returns their pseudo.
 *
 * @param id The ID of the user.
 * @return The pseudo of the user, or NULL if the user was not found.
 */
pseudo_t *get_pseudo(uint16_t id);

/**
 * @brief Returns the number of registered users.
 *
 * This function returns the number of registered users,
 * i.e. the length of the global array known_users.
 *
 * @return The number of registered users.
 */
size_t get_nb_user(void);

/* --------------------------------------------- */

/**
 * @brief Creates a new post in a specified feed with 
 * 	  the given data and author.
 *
 * The post is then appended to the feed's posts array.
 *
 * @param pseudo The author's pseudo.
 * @param datalen The length of the data content.
 * @param data A pointer to the data content.
 * @param feed_number Number of the feed to which the post will be added.
 * @return 0 if success, or 1 if the append to the feed's posts array failed.
 */
int post_new(pseudo_t pseudo, uint8_t datalen,
	     char *data, size_t feed_number);

/* --------------------------------------------- */

/**
 * @brief Creates a new feed with the given creator.
 *
 * This function creates a new feed with the given creator, initializes its
 * arrays for posts and subscribers, and adds it to the global array of feed.
 *
 * @param creator The pseudo of the user who created the feed.
 * @return 0 if successful, or 1 if the feed could not be created.
 */
uint8_t feed_new(pseudo_t creator);

/**
 * @brief Gets a pointer to a Feed struct corresponding to
 * 	  a specified feed number.
 *
 * @param feed_number The number of the feed to get.
 * @return A pointer to the feed struct, or NULL if the feed number 
 * 	   is out of bounds.
 */
Feed *get_feeds(size_t feed_number);

/**
 * @brief Returns the number of feeds in the server.
 *
 * This function returns the number of feeds that
 * have been created on the server.
 *
 * @return The number of feeds in the server.
 */
size_t get_feeds_count(void);

/* --------------------------------------------- */

uint8_t notif_new(size_t nbfeed, char *addr, uint16_t port,
		  int fd, SA_IN6 sock_addr);

NotificationsInfos *get_notif_info(size_t feed_number);

NotificationsInfos *get_feed_notif_info(uint16_t nbfeed);

size_t get_subscribe_count(void);

/* --------------------------------------------- */

uint8_t transfer_new(uint16_t id, uint16_t feed_number, const char *file_name);

uint8_t add_packet(uint16_t id, uint16_t numblock, char *data, size_t nbytes);

void id_clear_transfer(uint16_t id);

char *get_tranfer_file_path(uint16_t id);

void check_transfers_timeout(void);

/* -------------------------------------------------------------------------- */

#endif /* DATA_H */
