#ifndef USER_INPUT_H
#define USER_INPUT_H


/* -------------------------------- INCLUDES -------------------------------- */

#include <sys/types.h>

#include "user/user.h"
#include "network/request.h"

/* -------------------------------- DEFINES --------------------------------- */

#define INPUT_SIZE	512
#define MSG_SIZE	256

/* -------------------------------- FUNCTIONS ------------------------------- */

void readstring(const char* msg, char* input, size_t input_size);
int readint(const char* msg);

void feed_number_manager(coderq_t rq_type, uint16_t* feed_number);
void count_number_manager(coderq_t rq_type, uint16_t* count);
size_t pseudo_manager(char* input);
uint8_t data_manager(coderq_t rq_type, char* input);
uint8_t file_manager(char *file_name);

/* -------------------------------------------------------------------------- */

#endif /* USER_INPUT_H */
