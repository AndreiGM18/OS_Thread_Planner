/* SPDX-License-Identifier: EUPL-1.2 */
/* Copyright Mitran Andrei-Gabriel 2022 */

#ifndef THREAD_H_
#define THREAD_H_

#include "utils.h"
#include "util/so_scheduler.h"
#include "queue.h"

/* For the start routine */
struct parameter_t {
	uint priority;
	so_handler *func;
};

/* Thread implementation */
struct thread_t {
	tid_t tid;
	uint priority;
	so_handler *func;
	int time;
	bool is_waiting;
	int io;
};

/* Count how many threads in the queue have a specified priority */
uint cnt_threads_with_priority(struct queue_t *q, int priority);

/* Removes a specified thread from the queue */
void remove_thread(struct queue_t *q, struct thread_t *thread);

/* Reorders the nodes so that all threads that are not waiting
 * are after those that are
 */
void reorder(struct queue_t *q);

/* Sets the is_waiting field to false for all threads waiting for a given io
 * Also returns the number of threads woken up by io
 */
uint wake_up(struct queue_t *q, uint io);

/* Returns the first thread that is not waiting */
struct thread_t *front_not_waiting(struct queue_t *q);

#endif
