/**
 * @file node.h
 * @brief Contains prototypes for initialize and free a node struct.
 */

#ifndef NODE_H
#define NODE_H


/* -------------------------------- INCLUDE --------------------------------- */

#include <stddef.h>

/* -------------------------------- STRUCTURE ------------------------------- */

/**
 * @brief Node struct used in linked lists.
 */
typedef struct node
{
	void *data;
	struct node *next;
} Node;

/* -------------------------------- FUNCTIONS ------------------------------- */

/**
 * @brief Initializes a node with given data.
 *
 * @param data Pointer to the data to store in the node
 * @param data_size Size of the data to store
 * @return A pointer to the newly created node or NULL on error
 */
Node* node_init(void *data, size_t data_size);


/**
 * @brief Frees the memory occupied by a node and its data.
 *
 * @param node Pointer to the node to free.
 */
void node_free(Node *node);

/* -------------------------------------------------------------------------- */

#endif /* NODE_H */
