/**
 * @file queue.h
 * @brief Contains prototypes for a queue data structure.
 */

#ifndef QUEUE_H
#define QUEUE_H

/* -------------------------------- INCLUDES -------------------------------- */

#include "data_structures/node.h"

/* -------------------------------- STRUCTURE ------------------------------- */

typedef struct queue
{
	Node* head;
	Node* tail;
	size_t data_size;
	size_t length;

	int (*enqueue) (struct queue* queue, void* data);
	void* (*dequeue) (struct queue* queue);
} Queue;

/* -------------------------------- FUNCTIONS ------------------------------- */

/**
 * @brief Initializes a new queue.
 *
 * @param data_size The size of the data that will be stored
 * 	  in each node of the queue.
 * @return The newly initialized queue.
 */
Queue queue_init(size_t data_size);


/**
 * @brief Frees the memory used by a queue.
 *
 * @param queue A pointer to the queue to be freed.
 */
void queue_free(Queue* queue);

/* -------------------------------------------------------------------------- */

#endif /* QUEUE_H */
