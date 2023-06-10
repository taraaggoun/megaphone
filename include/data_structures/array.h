/**
 * @file array.h
 * @brief Prototypes of a dynamic array data structure.
 */

#ifndef ARRAY_H
#define ARRAY_H


/* -------------------------------- INCLUDE --------------------------------- */

#include <stddef.h>
#include <stdint.h>

/* -------------------------------- STRUCTURE ------------------------------- */

/**
 * @brief An implementation of a dynamic array data structure.
 */
typedef struct array
{
	size_t data_size;
	void *data;

	size_t length;
	size_t capacity;

	int (*append) (struct array *array, void *elt);
	void * (*get) (struct array *array, size_t i);
	int (*set) (struct array *array, size_t i, void *elt);
} Array;

/* -------------------------------- FUNCTIONS ------------------------------- */

uint8_t array_new(Array *array, size_t data_size, size_t capacity);


/**
 * @brief Frees the memory used by a dynamic array.
 *
 * @param array The dynamic array to free.
 */
void array_free(Array *array);
/* -------------------------------------------------------------------------- */

#endif /* ARRAY_H */
