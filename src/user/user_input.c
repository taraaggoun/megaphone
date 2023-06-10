#include "user/user_input.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "system/logger.h"

#define ACCEPTED_CHAR 	  ".-_"
#define MAX_FILE_SIZE     (1 << 25)

#define PROMPT_SIZE    	  (INPUT_SIZE * 2)
#define PROMPT_END        "> "

/* ---------------------------- PRIVATE FUNCTION ---------------------------- */

static uint8_t is_valid_pseudo(pseudo_t pseudo)
{
	int i = 0;
	while (i < PSEUDO_LEN) {
		if (isalnum(pseudo[i]) == 0 &&
		    strrchr(ACCEPTED_CHAR, pseudo[i]) == NULL)
		    return 0;
		
		if (pseudo[i] == '\0')
			break;
		i++;
	}
	if (i == 1 && isalpha(pseudo[0]) == 0)
		return 0;

	return 1;
}

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

void readstring(const char* msg, char* input, size_t input_size)
{
	char prompt[PROMPT_SIZE];

	memset(input, 0, input_size);
	memset(prompt, 0, PROMPT_SIZE);
	strcat(prompt, PROMPT_END);

	char* line = NULL;
	while (1) {
		log_msg(BLUE, "%s", msg);
		line = readline(prompt);
		/* EOF receive */
		if (line == NULL)
			exit(EXIT_SUCCESS);
		/* blank line */
		if (*line == 0) {
			free(line);
			continue;
		}
		if (strlen(line) < input_size)
			break;

		free(line);
		logerror("Input too long (%ld max)", input_size);
	}
	add_history(line);
	strcpy(input, line);
	free(line);

	printf("\n");
}

int readint(const char* msg)
{
	long input_integer;
	char input[INPUT_SIZE];

	while (1) {
		readstring(msg, input, INPUT_SIZE);

		char *endptr;
		input_integer = strtol(input, &endptr, 10);
		if (endptr != input && endptr[0] == 0)
			break;

		logerror("Input is not an integer\n");
	}

	return (int) input_integer;
}


void feed_number_manager(coderq_t rq_type, uint16_t *feed_number)
{
	char msg[MSG_SIZE] = "Enter the feed where you want "
			     "to execute your action:";

	if (rq_type == NEWPOST || rq_type == UPLOAD)
		strcat(msg, "\t(0 for a new one)");
	if (rq_type == LASTPOSTS)
		strcat(msg, "\t(0 for all)");

	char e_msg[MSG_SIZE];
	sprintf(e_msg, "The feed number must be an integer between %d and 65536\n", rq_type == (DOWNLOAD || SUBSCRIBE));

	while (1) {
		int number = readint(msg);
		if (number < UINT16_MAX) {
			if (!(number == 0 && (rq_type == DOWNLOAD || rq_type == SUBSCRIBE))) {
				*feed_number = (uint16_t) number;
				return;
			}
		}
		logerror("%s\n", e_msg);
	}
}

void count_number_manager(coderq_t rq_type, uint16_t *count)
{
	if (rq_type != LASTPOSTS) {
		*count = 0;
		return;
	}

	const char *msg = "Enter the number of post you want to"
			  "consult: \t(0 for all)";
	const char *e_msg = "the number must be an integer "
			    "between 0 and 65536 \n";
	while (1) {
		int number = readint(msg);
		if (number < UINT16_MAX) {
			*count = (uint16_t) number;
			return;
		}
		logerror("%s\n", e_msg);
	}
}

size_t pseudo_manager(char* input)
{
	const char msg[]   = "Enter your username:";

	const char e_msg[] = "the username must contain a maximum of"
			     " 10 characters and have at least one letter.\n"
			     "The only characters allowed are letters, numbers"
			     " and " ACCEPTED_CHAR "\n";
	size_t pseudo_len;
	while (1) {
		memset(input, 0, INPUT_SIZE);
		readstring(msg, input, INPUT_SIZE);

		size_t input_len = strlen(input);

		if (input_len <= PSEUDO_LEN && is_valid_pseudo(input)) {
			pseudo_len = input_len;
			break;
		}

		logerror("%s\n", e_msg);
	}

	return pseudo_len;
}

uint8_t data_manager(coderq_t rq_type, char *input)
{
	char msg[MSG_SIZE];
	memset(msg, 0, MSG_SIZE);

	memset(input, 0, INPUT_SIZE);
	uint8_t datalen = 0;

	size_t input_size = INPUT_SIZE;
	if (rq_type == NEWPOST)
		strcpy(msg, "Enter your post (255 characters maximum): ");

	else if (rq_type == UPLOAD) {
		strcpy(msg, "Enter the path of the file");
		strcat(msg, " (230 characters maximum): ");
		input_size = 230;
	}

	else if (rq_type == DOWNLOAD) {
		strcpy(msg, "Enter the name of the file");
		strcat(msg, " (230 characters maximum): ");
		input_size = 230;
	}
	
	if (msg[0] != '\0') {
		readstring(msg, input , input_size);
		datalen = (uint8_t) strlen(input);
	}

	return datalen;
}

uint8_t file_manager(char *file_name)
{
	int fd;
	while(1) {
		while ((fd = open(file_name, O_RDONLY)) < 0) {
			logerror("No such file or directory, please enter a correct path file");
			memset(file_name, 0, INPUT_SIZE);
			readstring("Enter the path of the file", file_name, INPUT_SIZE);
		}
		struct stat st;
		if (fstat(fd, &st) < 0) {
			perror("stat");
			return 1;
		}
		if (st.st_size <= MAX_FILE_SIZE && st.st_size != 0)
			break;

		if (st.st_size == 0)
			logerror("File size is nul");
		else
			logerror("File size is too large (max 33 mo)");

		memset(file_name, 0, INPUT_SIZE);
		readstring("Enter the path of the file", file_name, INPUT_SIZE);
	}
	close(fd);
	return 0;
}

/* -------------------------------------------------------------------------- */
