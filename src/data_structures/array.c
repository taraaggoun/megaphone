/**
 * @file array.c
 * @brief Implementation of a dynamic array data structure.
 */

#include "data_structures/array.h"

#include <stdlib.h>
#include <string.h>


#define INIT_CAP 8


/* ---------------------------- PRIVATE FUNCTIONS --------------------------- */

static void *srealloc(void *data, size_t size)
{
	void *new_data = realloc(data, size);
	if (new_data != NULL)
		return new_data;

	free(data);
	return NULL;
}

/**
 * @brief Appends an element to a dynamic array.
 *
 * If the capacity of the array is not sufficient to hold the new element,
 * the array will be resized to double its current capacity.
 *
 * @param array The dynamic array to append to.
 * @param element The element to append.
 * @return 0 on success, 1 on failure.
 */
static int append(Array *array, void *element)
{
	if (array->length == array->capacity) {
		array->capacity *= 2;

		size_t new_size = array->capacity * array->data_size;
		array->data = srealloc(array->data, new_size);
		if (array->data == NULL)
			return 1;
	}

	char *dst = &((char *) array->data)[array->data_size * array->length];
	memcpy(dst, element, array->data_size);

	array->length++;

	return 0;
}


/**
 * @brief Retrieves an element from a dynamic array.
 *
 * If the given index is greater than or equal to the length of the array,
 * NULL is returned.
 *
 * @param array The dynamic array to retrieve the element from.
 * @param i The index of the element to retrieve.
 * @return A pointer to the retrieved element,
 * 	   or NULL if the index is out of bounds.
 */
static void *get(Array *array, size_t i)
{
	if (i > array->length)
		return NULL;

	return &((char *) array->data)[array->data_size * i];
}

static int set(Array *array, size_t i, void *element)
{
	if (i >= array->capacity) {
		array->capacity = i + 1;
		array->length = i + 1;

		size_t new_size = array->capacity * array->data_size;
		array->data = srealloc(array->data, new_size);
		if (array->data == NULL)
			return 1;
	}

	char *dst = &((char *) array->data)[array->data_size * i];
	memcpy(dst, element, array->data_size);
	if (i >= array->length)
		array->length = i + 1;

	return 0;
}

/* ---------------------------- PUBLIC FUNCTIONS ---------------------------- */


uint8_t array_new(Array *array, size_t data_size, size_t capacity)
{
	array->data_size = data_size;
	array->capacity = (capacity < INIT_CAP) ? INIT_CAP : capacity;

	array->data = calloc(array->capacity, array->data_size);
	if (array->data == NULL)
		return 1;

	array->length = 0;

	array->append = append;
	array->get = get;
	array->set = set;

	return 0;
}


void array_free(Array *array)
{
	free(array->data);
	array->data = NULL;
}

/* -------------------------------------------------------------------------- */
