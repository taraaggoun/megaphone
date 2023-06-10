#include "system/thread_pool.h"

#include <stdlib.h>

#include "system/logger.h"

/* ---------------------------- PRIVATE FUNCTIONS --------------------------- */

static void* thread_pool_job(void* arg)
{
	ThreadPool *tpool = arg;
	ThreadJob *tjob;

	while (1) {
		pthread_mutex_lock(&tpool->q_mutex);

		/* On attend qu'il y ai des jobs à executer */
		while (tpool->jobs.head == NULL && tpool->active)
			pthread_cond_wait(&tpool->q_cond, &tpool->q_mutex);

		if (!tpool->active)
			break;

		Node *job_node = tpool->jobs.dequeue(&tpool->jobs);
		pthread_mutex_unlock(&tpool->q_mutex);

		/* Jobs recuperer, on l'execute */
		if (job_node != NULL) {
			tjob = job_node->data;
			if (tjob->job(tjob->arg) == NULL) {
				tpool->active = 0;
				break;
			}
			node_free(job_node);
		}
	}

	thread_pool_shutdown(tpool);
	exit(EXIT_FAILURE);
}


static void add_job(ThreadPool *tpool, ThreadJob *tjob)
{
	pthread_mutex_lock(&tpool->q_mutex);
    	tpool->jobs.enqueue(&tpool->jobs, tjob);

	/* On signal les autres threads qu'il y'a un job */
	pthread_cond_signal(&tpool->q_cond);
	pthread_mutex_unlock(&tpool->q_mutex);
}

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

ThreadPool *thread_pool_init(uint8_t pool_size)
{
	if (pool_size < 3)
		pool_size = 3;

	ThreadPool *thread_pool = calloc(1, sizeof(*thread_pool));
	if (thread_pool == NULL)
		return NULL;

	thread_pool->active = 1;
	thread_pool->pool_size = pool_size;

   	pthread_mutex_init(&thread_pool->q_mutex, NULL);
   	pthread_cond_init(&thread_pool->q_cond, NULL);

	thread_pool->jobs = queue_init(sizeof(ThreadJob));
	thread_pool->threads = calloc(pool_size, sizeof(pthread_t));
	if (thread_pool->threads == NULL) {
		free(thread_pool);
		return NULL;
	}

	for (int i = 0; i < pool_size; i++) {
		if (pthread_create(&thread_pool->threads[i], NULL,
				   thread_pool_job, thread_pool)) {
			free(thread_pool->threads);
			free(thread_pool);
	    		return NULL;
		}
	}

	thread_pool->add_job = add_job;
	return thread_pool;
}


void thread_pool_shutdown(ThreadPool *thread_pool)
{
	pthread_mutex_lock(&thread_pool->q_mutex);

	thread_pool->active = 0;

	pthread_cond_broadcast(&thread_pool->q_cond);
	pthread_mutex_unlock(&thread_pool->q_mutex);

	for (int i = 0; i < thread_pool->pool_size; i++)
		pthread_join(thread_pool->threads[i], NULL);

	pthread_mutex_destroy(&thread_pool->q_mutex);
	pthread_cond_destroy(&thread_pool->q_cond);

	queue_free(&thread_pool->jobs);
	free(thread_pool->threads);
	free(thread_pool);
}

void thread_pool_wait(ThreadPool *thread_pool)
{
	for (uint8_t i = 0; i < thread_pool->pool_size; i++) {
		pthread_join(thread_pool->threads[i], NULL);
	}
}

/* -------------------------------------------------------------------------- */
