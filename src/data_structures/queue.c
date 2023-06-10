/**
 * @file queue.c
 * @brief This file contains the implementation of a queue data structure.
 */

#include "data_structures/queue.h"

#include <string.h>


/* ---------------------------- PRIVATE FUNCTIONS --------------------------- */

/**
 * @brief Enqueues a new node to the tail of the queue with the provided data.
 *
 * @param queue Pointer to the queue to which the data is to be enqueued.
 * @param data Pointer to the data to be enqueued.
 * @return Returns 0 on success, 1 on failure to allocate memory.
 */
static int enqueue(Queue* queue, void* data)
{
	if (queue == NULL)
		return 1;

	Node *new = node_init(data, queue->data_size);
	if (new == NULL)
		return 1;

	if (queue->length == 0) {
		queue->head = new;
		queue->tail = new;
	} else {
		queue->tail->next = new;
		queue->tail = new;
	}

	queue->length++;
	return 0;
}


/**
 * @brief Dequeues an element from the given queue.
 *
 * @param queue Pointer to the queue to dequeue an element from.
 * @param data Pointer to the memory where the dequeued element will be stored.
 * @return 0 if the dequeue operation was successful, 1 if the queue was empty.
 */
static void* dequeue(Queue* queue)
{
	if (queue == NULL || queue->length == 0)
		return NULL;

	Node* node = queue->head;
	if (queue->length > 1) {
		queue->head = queue->head->next;
	} else {
		queue->head = NULL;
		queue->tail = NULL;
	}

	queue->length--;
	node->next = NULL;

	return node;
}

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

Queue queue_init(size_t data_size)
{
	Queue queue;

	queue.head = NULL;
	queue.tail = NULL;
	queue.data_size = data_size;
	queue.length = 0;

	queue.enqueue = enqueue;
	queue.dequeue = dequeue;

	return queue;
}


void queue_free(Queue* queue)
{
	Node* tmp = NULL;
	while (queue->head != NULL) {
		tmp = queue->head;
		queue->head = queue->head->next;
		node_free(tmp);
	}
}

/* -------------------------------------------------------------------------- */
