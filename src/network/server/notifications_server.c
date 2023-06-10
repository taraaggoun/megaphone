#include "network/server/notifications_server.h"

#include <unistd.h>
#include <string.h>

#include "data_structures/array.h"

#include "network/request.h"
#include "network/server/data.h"
#include "network/server/network.h"


/* ---------------------------- PUBLIC FUNCTION ----------------------------- */

void *notifications_loop(__attribute__((unused)) void *args)
{
	while (1) {
		sleep(120);
		size_t nb_feed = get_subscribe_count();
		for (size_t i = 0; i < nb_feed; i++) {
			Array a_serverrq; 
			if (array_new(&a_serverrq, sizeof(ServerRQ), 0))
				return NULL;
			NotificationsInfos *infos = get_notif_info(i);
			Feed *feed = get_feeds(infos->nbfeed - 1);
			Array *posts = &feed->posts;

			size_t start_post = infos->last_send_post;
			size_t posts_count = posts->length - start_post;

			for (size_t j = 0; j < posts_count; j++) {
				Post *post = posts->get(posts, start_post + j);
				ServerRQ serverrq;
				char data[NT_DATA_LEN];
				memset(data, 0, NT_DATA_LEN);
				strncpy(data, post->data, NT_DATA_LEN);
				serverrq.nt = serverrq_nt_new(SUBSCRIBE, (uint16_t) nb_feed, &post->pseudo, data);
				if (a_serverrq.append(&a_serverrq, &serverrq))
					return NULL;
			}
			if(send_notif(infos->mult_infos.sock_fd, a_serverrq.data, (int) posts_count, &infos->mult_infos.sock_addr))
				return NULL;
			infos->last_send_post = posts->length;
		}
	}
}

/* -------------------------------------------------------------------------- */
