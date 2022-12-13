/* SPDX-License-Identifier: EUPL-1.2 */
/* Copyright Mitran Andrei-Gabriel 2022 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include "utils.h"

/* Node struct for the queue, data is generic */
struct node_t {
	void *data;
	uint priority;
	struct node_t *next;
};

/* Queue, created using a linked list approach (highest priority is first) */
struct queue_t {
	struct node_t *front;
	/* Remembers the data's size */
	uint data_size;
};

/* Creates a queue */
struct queue_t *create(uint data_size);

/* Creates a node for the queue, generic data */
struct node_t *create_node(void *data, uint priority, uint data_size);

/* Checks if the queue is empty */
bool is_empty(struct queue_t *q);

/* Adds a node in which data is stored to the queue */
void enqueue(struct queue_t *q, void *data, uint priority);

/* Frees all the queue's resources */
void free_q(struct queue_t *q, void (*free_func)(void *));

#endif
