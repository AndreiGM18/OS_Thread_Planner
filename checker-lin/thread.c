// SPDX-License-Identifier: EUPL-1.2
/* Copyright Mitran Andrei-Gabriel 2022 */

#include <stdio.h>
#include <string.h>

#include "thread.h"
#include "utils.h"
#include "queue.h"

uint cnt_threads_with_priority(struct queue_t *q, int priority)
{
	DIE(!q, "queue missing");

	if (is_empty(q) == TRUE)
		return 0;

	struct node_t *it = q->front;
	uint cnt = 0;

	/* Goes through all nodes and checks if the thread is waiting */
	while (it && it->priority == priority) {
		if (((struct thread_t *)it->data)->is_waiting == FALSE)
			++cnt;

		it = it->next;
	}

	return cnt;
}

void remove_thread(struct queue_t *q, struct thread_t *thread)
{
	DIE(!q, "queue missing");

	if (is_empty(q) == TRUE)
		return;

	/* The node may be the first one */
	if (((struct thread_t *)q->front->data)->tid == thread->tid) {
		struct node_t *next = q->front->next;

		free(q->front->data);
		free(q->front);

		q->front = next;
		return;
	}

	struct node_t *it = q->front, *prev;

	/* Checks all remaining nodes */
	while (it) {
		/* Remembers the previous node, so the new link can be established */
		prev = it;
		it = it->next;

		if (((struct thread_t *)it->data)->tid == thread->tid) {
			prev->next = it->next;
			free(it->data);
			free(it);

			return;
		}
	}

}

void reorder(struct queue_t *q)
{
	DIE(!q, "queue missing");

	/* Nothing to reorder */
	if (is_empty(q) == TRUE || q->front->next == NULL)
		return;

	struct node_t *it = q->front;

	/* Skips all waiting nodes*/
	while (it && ((struct thread_t *)it->data)->is_waiting == TRUE)
		it = it->next;

	/* If there is more than one thread with the first non waiting thread's
	 * priority
	 */
	if (cnt_threads_with_priority(q, it->priority) > 1) {
		struct thread_t thread = *(struct thread_t *)it->data;

		/* Goes to the next node */
		it = it->next;

		/* Removes, then adds the thread back into the queue, so it is now
		 * at the end of all the threads with the same priority
		 */
		remove_thread(q, &thread);
		enqueue(q, &thread, thread.priority);

		/* Repeats the process */
		while (it && it->priority == thread.priority) {
			struct node_t *next = it->next;

			/* If a non waiting thread is found, stops*/
			if (((struct thread_t *)it->data)->is_waiting == FALSE)
				break;

			struct thread_t found = *(struct thread_t *)it->data;

			remove_thread(q, &found);
			enqueue(q, &found, found.priority);

			it = next;
		}
	}
}

uint wake_up(struct queue_t *q, uint io)
{
	DIE(!q, "queue missing");

	if (is_empty(q) == TRUE)
		return 0;

	struct node_t *it = q->front;

	uint cnt = 0;

	/* Goes through all nodes */
	while (it) {
		struct thread_t *thread = (struct thread_t *)it->data;

		/* If a thread that is waiting for io is found, wakes it up
		 * Also adds 1 to the counter
		 */
		if (thread->is_waiting == TRUE && thread->io == io) {
			++cnt;
			thread->is_waiting = FALSE;
		}

		it = it->next;
	}

	/* Returns how many threads were woken up by the io */
	return cnt;
}

struct thread_t *front_not_waiting(struct queue_t *q)
{
	if (is_empty(q) == TRUE)
		return NULL;

	struct node_t *it = q->front;

	/* Gets the first thread that is not waiting for an io */
	while (it) {
		if (((struct thread_t *)it->data)->is_waiting == FALSE)
			return (struct thread_t *)it->data;

		it = it->next;
	}

	return NULL;
}
