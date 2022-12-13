// SPDX-License-Identifier: EUPL-1.2
/* Copyright Mitran Andrei-Gabriel 2022 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"
#include "queue.h"

struct queue_t *create(uint data_size)
{
	struct queue_t *q = (struct queue_t *)malloc(sizeof(struct queue_t));

	DIE(!q, "queue malloc() failed");

	/* The queue is initially empty */
	q->front = NULL;

	q->data_size = data_size;

	return q;
}

struct node_t *create_node(void *data, uint priority, uint data_size)
{
	struct node_t *new = (struct node_t *)malloc(sizeof(struct node_t));

	DIE(!new, "new node malloc() failed");

	new->data = malloc(data_size);
	DIE(!new, "new node data malloc() failed");

	/* Copies the data in the void *data field */
	memcpy(new->data, data, data_size);

	/* Sets the priority */
	new->priority = priority;

	new->next = NULL;

	return new;
}

bool is_empty(struct queue_t *q)
{
	DIE(!q, "queue missing");

	return q->front == NULL;
}

void enqueue(struct queue_t *q, void *data, uint priority)
{
	DIE(!q, "queue missing");
	DIE(!data, "data missing");

	struct node_t *new = create_node(data, priority, q->data_size);

	/* The first node is to be added, since the queue is empty */
	if (is_empty(q) == TRUE) {
		new->next = NULL;

		q->front = new;

		return;
	}

	if (q->front->priority < priority) {
		/* If the added data has a higher priority than front, put it first */
		new->next = q->front;

		/* The new front is now the new node */
		q->front = new;
	} else {
		struct node_t *it = q->front, *prev;

		/* Find where the new data's priority is less than all other data's
		 * priorities
		 */
		while (it && it->priority >= priority) {
			/* The new node is to be added last */
			if (it->next == NULL) {
				it->next = new;

				new->next = NULL;

				return;
			}

			/* Goes through all nodes, remembering the previous node */
			prev = it;
			it = it->next;
		}

		/* Adds the new node */
		prev->next = new;

		new->next = it;
	}
}

void free_q(struct queue_t *q, void (*free_func)(void *))
{
	struct node_t *it = q->front;

	while (it) {
		struct node_t *aux = it;

		it = it->next;

		/* Custom free function for data if required */
		free_func(aux->data);

		free(aux);
	}

	free(q);
}
