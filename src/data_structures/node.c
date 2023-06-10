/**
 * @file node.c
 * @brief Contains functions for initialize and free a node struct.
 */

#include "data_structures/node.h"

#include <stdlib.h>
#include <string.h>


/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */

Node *node_init(void *data, size_t data_size)
{
	Node *node = malloc(sizeof(Node));
	if (node == NULL)
		return NULL;

	node->data = malloc(data_size);
	if (node->data == NULL) {
		free(node);
		return NULL;
	}

	memcpy(node->data, data, data_size);
	node->next = NULL;
	return node;
}


void node_free(Node *node)
{
	free(node->data);
	free(node);
}

/* -------------------------------------------------------------------------- */
