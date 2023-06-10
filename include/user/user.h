#ifndef USER_H
#define USER_H


/* -------------------------------- INCLUDE --------------------------------- */

#include <stdint.h>

/* --------------------------------- DEFINES -------------------------------- */

#define ID_MAX      ((1 << 11) - 1)	/* 2047 */
#define PSEUDO_LEN  10

typedef char        pseudo_t[PSEUDO_LEN];

/* -------------------------------- STRUCTURE ------------------------------- */

typedef struct
{
       	uint16_t id;
	pseudo_t pseudo;
} User;

/* -------------------------------- FUNCTIONS ------------------------------- */

uint16_t get_user_id(void);

void set_user_id(uint16_t id);

User user_init(uint16_t id, pseudo_t pseudo);

uint8_t id_exist(uint16_t id);

uint8_t  show_accounts(void);

void unload_accounts(void);

uint8_t  load_accounts(void);

uint8_t add_account(uint16_t id, pseudo_t pseudo);

/* -------------------------------------------------------------------------- */

#endif /* USER_H */
