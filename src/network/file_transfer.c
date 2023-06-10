#include "network/file_transfer.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "network/request_macros.h"


FileTransferInfos transfer_init(const char *file_name, uint16_t feed_number)
{
	FileTransferInfos infos;
	memset(&infos, 0, sizeof(infos));

	infos.active = 1;
	infos.begin = time(NULL);

	memcpy(infos.file_path, file_name, MAX_DATALEN);
	infos.feed_number = feed_number;

	array_new(&infos.file_data, FILE_PACKET_SIZE, 0);
	infos.last_packet = 0;
	infos.file_size = 0;

	return infos;
}


uint8_t file_exist(char *file_name, uint16_t feed_number)
{
	char path_file[MAX_DATALEN];
	memset(path_file, 0, MAX_DATALEN);
	sprintf(path_file, "%s/%u/%s", UPLOAD_FILES_PATH, feed_number, file_name);

	int fd;
	if ((fd = open(path_file, O_RDONLY)) < 0)
		return 0;

	close(fd);
	memcpy(file_name, path_file, strlen(path_file));
	return 1;
}


void transfer_clear(FileTransferInfos *infos)
{
	array_free(&infos->file_data);
	memset(infos, 0, sizeof(*infos));
}
