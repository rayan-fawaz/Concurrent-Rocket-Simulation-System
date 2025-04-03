#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Resource functions */

/**
 * Creates a new `Resource` object.
 *
 * Allocates memory for a new `Resource` and initializes its fields.
 * The `name` is dynamically allocated.
 *
 * @param[out] resource      Pointer to the `Resource*` to be allocated and initialized.
 * @param[in]  name          Name of the resource (the string is copied).
 * @param[in]  amount        Initial amount of the resource.
 * @param[in]  max_capacity  Maximum capacity of the resource.
 */
void resource_create(Resource **resource, const char *name, int amount, int max_capacity) {
    if (!resource || !name) return;

    *resource = malloc(sizeof(Resource));
    if (!*resource) {
        fprintf(stderr, "Failed to allocate memory for Resource\n");
        exit(EXIT_FAILURE);
    }

    (*resource)->name = malloc(strlen(name) + 1);
    if (!(*resource)->name) {
        fprintf(stderr, "Failed to allocate memory for Resource name\n");
        free(*resource);
        exit(EXIT_FAILURE);
    }
    strcpy((*resource)->name, name);

    (*resource)->amount = amount;
    (*resource)->max_capacity = max_capacity;

    // Initialize semaphore
    sem_init(&(*resource)->mutex, 0, 1);
}

/**
 * Destroys a `Resource` object.
 *
 * Frees all memory associated with the `Resource`.
 *
 * @param[in,out] resource  Pointer to the `Resource` to be destroyed.
 */
void resource_destroy(Resource *resource) {
    if (!resource) return;

    // Destroy semaphore
    sem_destroy(&resource->mutex);

    free(resource->name);
    free(resource);
}

/* ResourceAmount functions */

/**
 * Initializes a `ResourceAmount` structure.
 *
 * Associates a `Resource` with a specific `amount`.
 *
 * @param[out] resource_amount  Pointer to the `ResourceAmount` to initialize.
 * @param[in]  resource         Pointer to the `Resource`.
 * @param[in]  amount           The amount associated with the `Resource`.
 */
void resource_amount_init(ResourceAmount *resource_amount, Resource *resource, int amount) {
    resource_amount->resource = resource;
    resource_amount->amount = amount;
}

/**
 * Initializes the `ResourceArray`.
 *
 * Allocates memory for the array of `Resource*` pointers and sets initial values.
 *
 * @param[out] array  Pointer to the `ResourceArray` to initialize.
 */
void resource_array_init(ResourceArray *array) {
    if (!array) return;

    array->resources = malloc(sizeof(Resource *) * 1);
    if (!array->resources) {
        fprintf(stderr, "Failed to allocate memory for ResourceArray\n");
        exit(EXIT_FAILURE);
    }

    array->capacity = 1;
    array->size = 0;
}

/**
 * Cleans up the `ResourceArray` by destroying all resources and freeing memory.
 *
 * Iterates through the array, calls `resource_destroy` on each `Resource`,
 * and frees the array memory.
 *
 * @param[in,out] array  Pointer to the `ResourceArray` to clean.
 */
void resource_array_clean(ResourceArray *array) {
    if (!array) return;

    for (int i = 0; i < array->size; i++) {
        resource_destroy(array->resources[i]);
    }

    free(array->resources);
    array->resources = NULL;
    array->capacity = 0;
    array->size = 0;
}

/**
 * Adds a `Resource` to the `ResourceArray`, resizing if necessary (doubling the size).
 *
 * Resizes the array when the capacity is reached and adds the new `Resource`.
 * Use of realloc is NOT permitted.
 * 
 * @param[in,out] array     Pointer to the `ResourceArray`.
 * @param[in]     resource  Pointer to the `Resource` to add.
 */
void resource_array_add(ResourceArray *array, Resource *resource) {
    if (!array || !resource) return;

    if (array->size >= array->capacity) {
        int new_capacity = array->capacity * 2;
        Resource **new_resources = malloc(sizeof(Resource *) * new_capacity);
        if (!new_resources) {
            fprintf(stderr, "Failed to reallocate memory for ResourceArray\n");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < array->size; i++) {
            new_resources[i] = array->resources[i];
        }

        free(array->resources);
        array->resources = new_resources;
        array->capacity = new_capacity;
    }

    array->resources[array->size] = resource;
    array->size++;
}