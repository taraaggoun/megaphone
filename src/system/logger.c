#include "system/logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#define RED    "\001\033[0;91m\002"
#define GREEN  "\001\033[0;92m\002"
#define YELLOW "\001\033[0;93m\002"

uint16_t d_errno = NOERROR;

/* ---------------------------- PUBLIC FUNCTIONS --------------------------- */

#define RESET  "\001\033[00m\002"

int is_error(int rq_type)
{
	return rq_type == ERR_NOID || rq_type == ERR_FEEDNB ||
	       rq_type == ERR_NOFILE || rq_type == ERR_PSEUDO ||
	       rq_type == ERR_NOTCOMPLET || rq_type == ERR_FEEDMAX ||
	       rq_type == ERR_IDMAX;
}

const char* strerrno(void)
{
	switch (d_errno) {
		case ERR_NOID:
			return "Id not found";

		case ERR_FEEDNB:
			return "Feed number does not exist";

		case ERR_NOFILE:
			return "File not found";

		case ERR_PSEUDO:
			return "Bad pseudo format";

		case ERR_NOTCOMPLET:
			return "Incomplete request";

		case ERR_FEEDMAX:
			return "A new feed can't be create";

		case ERR_IDMAX:
			return "A new account can't be create";

		default:
			break;
	}

	return "None";
}

/* ------------------------------ Affichages ------------------------------ */

void log_msg(const char *color, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	dprintf(STDIN_FILENO, "%s", color);
	vdprintf(STDIN_FILENO, format, args);
	dprintf(STDIN_FILENO, "%s\n", RESET);

	va_end(args);
}

void logsuccess(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	dprintf(STDIN_FILENO, "%s", GREEN);
	vdprintf(STDIN_FILENO, format, args);
	dprintf(STDIN_FILENO, "%s\n", RESET);

	va_end(args);
}

void logerror(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	dprintf(STDERR_FILENO, "%s Error: ", RED);
	vdprintf(STDERR_FILENO, format, args);
	dprintf(STDERR_FILENO, "%s\n", RESET);

	va_end(args);
}

void debug_log(const char *format __attribute__((unused)), ...)
{
#ifdef DEBUG
	va_list args;
	va_start(args, format);

	dprintf(STDIN_FILENO, "%s", YELLOW);
	vdprintf(STDIN_FILENO, format, args);
	dprintf(STDIN_FILENO, "%s\n", RESET);

	va_end(args);
#endif
}


void debug_logerror(const char *format __attribute__((unused)), ...)
{
#ifdef DEBUG
	va_list args;
	va_start(args, format);

	dprintf(STDERR_FILENO, "%s", RED);
	vdprintf(STDERR_FILENO, format, args);
	dprintf(STDERR_FILENO, "%s\n", RESET);

	va_end(args);
#endif
}


/* -------------------- Logging message to the log file -------------------- */

#include <stdint.h>
#include <time.h>
#include <fcntl.h>

#define LOGFILENAME "res/server/mp.log"

static int logfd = 0;

void log_close(void)
{
	close(logfd);
}


void log_init(void)
{
	atexit(log_close);

	logfd = open(LOGFILENAME, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (logfd == -1)
		exit(EXIT_FAILURE);
}


void log_to_file(const char *format, ...)
{
	const uint8_t st_buflen = 32;

	time_t timestamp = time(NULL);
	struct tm *tm_info = localtime(&timestamp);
	char strdatestamp[st_buflen];
	char strtimestamp[st_buflen];

	strftime(strdatestamp, st_buflen, "%H:%M:%S", tm_info);
	strftime(strtimestamp, st_buflen, "%F (%a)", tm_info);

	dprintf(logfd, "%s, ", strdatestamp);
    	dprintf(logfd, "%s, ", strtimestamp);

	va_list args;
	va_start(args, format);

	vdprintf(logfd, format, args);

	va_end(args);
}

/* -------------------------------------------------------------------------- */
