#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

/* --------------------------------- DEFINES -------------------------------- */

#define NOERROR         0x00

#define ERR_NOID        0x19       /* id does not exist */
#define ERR_FEEDNB	0x1A	   /* Feed number does not exist*/
#define ERR_NOFILE	0x1B	   /* File does not exist*/
#define ERR_PSEUDO      0x1C       /* pseudo format */
#define ERR_NOTCOMPLET  0x1D	   /* request not complete */
#define ERR_FEEDMAX	0x1E	   /* No more feed number available */
#define ERR_IDMAX	0x1F	   /* No more ID available */

extern uint16_t d_errno;

#define BLUE   "\001\033[1;34m\002"
#define PURPLE "\001\033[1;35m\002"
#define CYAN   "\001\033[1;36m\002"

/* -------------------------------- FUNCTIONS ------------------------------- */

const char* strerrno(void);
int is_error(int rq_type);

/* Affichages basiques */
void log_msg(const char *color, const char *format, ...);
void logerror(const char *format, ...);
void logsuccess(const char *format, ...);

/* Affichages visible uniquement en debug */
void debug_log(const char *format, ...);
void debug_logerror(const char *format, ...);

/* Log du server */
void log_init(void);
void log_close(void);
void log_to_file(const char *format, ...);

/* -------------------------------------------------------------------------- */

#endif /* LOGGER_H */
