#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

// Helper functions just used by this C file to clean up our code
// Using static means they can't get linked into other files

static int system_convert(System *);
static void system_simulate_process_time(System *);
static int system_store_resources(System *);

/**
 * Creates a new `System` object.
 */
void system_create(System **system, const char *name, ResourceAmount consumed, ResourceAmount produced, int processing_time, EventQueue *event_queue) {
    if (!system || !name) return;

    *system = malloc(sizeof(System));
    if (!*system) {
        fprintf(stderr, "Failed to allocate memory for System\n");
        exit(EXIT_FAILURE);
    }

    (*system)->name = malloc(strlen(name) + 1);
    if (!(*system)->name) {
        fprintf(stderr, "Failed to allocate memory for System name\n");
        free(*system);
        exit(EXIT_FAILURE);
    }
    strcpy((*system)->name, name);

    (*system)->consumed = consumed;
    (*system)->produced = produced;
    (*system)->processing_time = processing_time;
    (*system)->event_queue = event_queue;
    (*system)->status = STANDARD;
    (*system)->amount_stored = 0;
}

/**
 * Destroys a `System` object.
 */
void system_destroy(System *system) {
    if (!system) return;

    free(system->name);
    free(system);
}

/**
 * Initializes the `SystemArray`.
 */
void system_array_init(SystemArray *array) {
    if (!array) return;

    array->systems = malloc(sizeof(System *) * 1);
    if (!array->systems) {
        fprintf(stderr, "Failed to allocate memory for SystemArray\n");
        exit(EXIT_FAILURE);
    }

    array->capacity = 1;
    array->size = 0;
}

void *system_thread(void *arg) {
    System *system = (System *)arg;
    while (system->status != TERMINATE) {
        system_run(system);
    }
    return NULL;
}

/**
 * Cleans up the `SystemArray` by destroying all systems and freeing memory.
 */
void system_array_clean(SystemArray *array) {
    if (!array) return;

    for (int i = 0; i < array->size; i++) {
        system_destroy(array->systems[i]);
    }

    free(array->systems);
    array->systems = NULL;
    array->capacity = 0;
    array->size = 0;
}

/**
 * Adds a `System` to the `SystemArray`, resizing if necessary (doubling the size).
 */
void system_array_add(SystemArray *array, System *system) {
    if (!array || !system) return;

    if (array->size >= array->capacity) {
        int new_capacity = array->capacity * 2;
        System **new_systems = malloc(sizeof(System *) * new_capacity);
        if (!new_systems) {
            fprintf(stderr, "Failed to reallocate memory for SystemArray\n");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < array->size; i++) {
            new_systems[i] = array->systems[i];
        }

        free(array->systems);
        array->systems = new_systems;
        array->capacity = new_capacity;
    }

    array->systems[array->size] = system;
    array->size++;
}

/**
 * Runs the main loop for a `System`.
 */
void system_run(System *system) {
    Event event;
    int result_status;

    if (system->amount_stored == 0) {
        result_status = system_convert(system);

        if (result_status != STATUS_OK) {
            event_init(&event, system, system->consumed.resource, result_status, PRIORITY_HIGH, system->consumed.resource->amount);
            event_queue_push(system->event_queue, &event);
            usleep(SYSTEM_WAIT_TIME * 1000);
        }
    }

    if (system->amount_stored > 0) {
        result_status = system_store_resources(system);

        if (result_status != STATUS_OK) {
            event_init(&event, system, system->produced.resource, result_status, PRIORITY_LOW, system->produced.resource->amount);
            event_queue_push(system->event_queue, &event);
            usleep(SYSTEM_WAIT_TIME * 1000);
        }
    }
}

/**
 * Converts resources in a `System`.
 */
static int system_convert(System *system) {
    int status;
    Resource *consumed_resource = system->consumed.resource;
    int amount_consumed = system->consumed.amount;

    if (consumed_resource == NULL) {
        status = STATUS_OK;
    } else {
        sem_wait(&consumed_resource->mutex);  // Lock the resource

        if (consumed_resource->amount >= amount_consumed) {
            consumed_resource->amount -= amount_consumed;
            status = STATUS_OK;
        } else {
            status = (consumed_resource->amount == 0) ? STATUS_EMPTY : STATUS_INSUFFICIENT;
        }

        sem_post(&consumed_resource->mutex);  // Unlock the resource
    }

    if (status == STATUS_OK) {
        system_simulate_process_time(system);

        if (system->produced.resource != NULL) {
            system->amount_stored += system->produced.amount;
        } else {
            system->amount_stored = 0;
        }
    }

    return status;
}

/**
 * Simulates the processing time for a `System`.
 */
static void system_simulate_process_time(System *system) {
    int adjusted_processing_time;

    // Adjust based on the current system status modifier
    switch (system->status) {
        case SLOW:
            adjusted_processing_time = system->processing_time * 2;
            break;
        case FAST:
            adjusted_processing_time = system->processing_time / 2;
            break;
        default:
            adjusted_processing_time = system->processing_time;
    }

    // Sleep for the required time
    usleep(adjusted_processing_time * 1000);
}

/**
 * Stores produced resources in a `System`.
 */
static int system_store_resources(System *system) {
    Resource *produced_resource = system->produced.resource;

    if (produced_resource == NULL || system->amount_stored == 0) {
        system->amount_stored = 0;
        return STATUS_OK;
    }

    sem_wait(&produced_resource->mutex);  // Lock the resource

    int available_space = produced_resource->max_capacity - produced_resource->amount;

    if (available_space >= system->amount_stored) {
        produced_resource->amount += system->amount_stored;
        system->amount_stored = 0;
    } else if (available_space > 0) {
        produced_resource->amount += available_space;
        system->amount_stored -= available_space;
    }

    sem_post(&produced_resource->mutex);  // Unlock the resource

    if (system->amount_stored == 0) {
        return STATUS_OK;
    }

    return STATUS_CAPACITY;
}
