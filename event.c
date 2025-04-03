#include "defs.h"
#include <stdlib.h>
#include <stdio.h>

/* Event functions */

/**
 * Initializes an `Event` structure.
 *
 * Sets up an `Event` with the provided system, resource, status, priority, and amount.
 *
 * @param[out] event     Pointer to the `Event` to initialize.
 * @param[in]  system    Pointer to the `System` that generated the event.
 * @param[in]  resource  Pointer to the `Resource` associated with the event.
 * @param[in]  status    Status code representing the event type.
 * @param[in]  priority  Priority level of the event.
 * @param[in]  amount    Amount related to the event (e.g., resource amount).
 */
void event_init(Event *event, System *system, Resource *resource, int status, int priority, int amount) {
    event->system = system;
    event->resource = resource;
    event->status = status;
    event->priority = priority;
    event->amount = amount;
}

/* EventQueue functions */

/**
 * Initializes the `EventQueue`.
 *
 * Sets up the queue for use, initializing any necessary data (e.g., semaphores when threading).
 *
 * @param[out] queue  Pointer to the `EventQueue` to initialize.
 */
void event_queue_init(EventQueue *queue) {
    if (!queue) return;
    queue->head = NULL;
    sem_init(&queue->mutex, 0, 1);  // Initialize the semaphore
}

/**
 * Cleans up the `EventQueue`.
 *
 * Frees any memory and resources associated with the `EventQueue`.
 * 
 * @param[in,out] queue  Pointer to the `EventQueue` to clean.
 */
void event_queue_clean(EventQueue *queue) {
    if (!queue) return;

    EventNode *current = queue->head;
    EventNode *next;

    while (current) {
        next = current->next;
        free(current);
        current = next;
    }

    sem_destroy(&queue->mutex);  // Destroy the semaphore

    queue->head = NULL;
}

/**
 * Pushes an `Event` onto the `EventQueue`.
 *
 * Adds the event to the queue in a thread-safe manner, maintaining priority order (highest first).
 *
 * @param[in,out] queue  Pointer to the `EventQueue`.
 * @param[in]     event  Pointer to the `Event` to push onto the queue.
 */
void event_queue_push(EventQueue *queue, const Event *event) {
    if (!queue || !event) return;

    sem_wait(&queue->mutex);  // Lock the queue

    // Allocate memory for the new EventNode
    EventNode *new_node = malloc(sizeof(EventNode));
    if (!new_node) {
        fprintf(stderr, "Failed to allocate memory for EventNode\n");
        sem_post(&queue->mutex);
        exit(EXIT_FAILURE);
    }

    // Copy the event data into the new node
    new_node->event = *event;
    new_node->next = NULL;

    // Insert the new node into the queue in priority order
    EventNode **current = &queue->head;

    while (*current && (*current)->event.priority >= event->priority) {
        current = &(*current)->next;
    }

    new_node->next = *current;
    *current = new_node;

    sem_post(&queue->mutex);  // Unlock the queue
}

/**
 * Pops an `Event` from the `EventQueue`.
 *
 * Removes the highest priority event from the queue in a thread-safe manner.
 *
 * @param[in,out] queue  Pointer to the `EventQueue`.
 * @param[out]    event  Pointer to the `Event` structure to store the popped event.
 * @return               Non-zero if an event was successfully popped; zero otherwise.
 */
int event_queue_pop(EventQueue *queue, Event *event) {
    if (!queue || !event) return 0;

    sem_wait(&queue->mutex);  // Lock the queue

    if (!queue->head) {
        sem_post(&queue->mutex);
        return 0;
    }

    // Remove the head node
    EventNode *node_to_remove = queue->head;
    *event = node_to_remove->event;
    queue->head = node_to_remove->next;

    free(node_to_remove);

    sem_post(&queue->mutex);  // Unlock the queue

    return 1;
}