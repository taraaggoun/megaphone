#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/* -------------------------------- INCLUDES -------------------------------- */

#include <stdint.h>
#include <pthread.h>

#include "data_structures/queue.h"

/* -------------------------------- STRUCTURES ------------------------------ */

typedef struct
{
	void *(*job) (void *arg);
	void *arg;
} ThreadJob;

typedef struct thread_pool
{
	uint8_t active;
	uint8_t pool_size;
	Queue jobs;

	pthread_t *threads;
	pthread_mutex_t q_mutex;
	pthread_cond_t q_cond;

	void (*add_job) (struct thread_pool* t_pool, ThreadJob *t_job);
} ThreadPool;

/* -------------------------------- FUNCTIONS ------------------------------- */

ThreadPool *thread_pool_init(uint8_t pool_size);

void thread_pool_wait(ThreadPool *thread_pool);

void thread_pool_shutdown(ThreadPool *thread_pool);

/* -------------------------------------------------------------------------- */

#endif /* THREAD_POOL_H */
