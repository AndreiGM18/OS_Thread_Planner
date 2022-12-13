/* SPDX-License-Identifier: EUPL-1.2 */
/* Copyright Mitran Andrei-Gabriel 2022 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <pthread.h>

#include "thread.h"
#include "queue.h"
#include "utils.h"

/* Scheduler implementation */
struct scheduler_t {
	uint time_quantum;
	uint io;
	/* Queue that stores thread_t threads */
	struct queue_t *q;
	/* Queue that stores tid_t of all the thread_t threads */
	struct queue_t *all_threads;
	/* Mutex for sync */
	pthread_mutex_t mutex;
	/* Waiting conditions for the mutex */
	pthread_cond_t ready;
	pthread_cond_t running;
	bool is_ready;
	tid_t current_tid;
	struct thread_t *current;
	bool is_finished;
};

/* Checks if the current thread matches the current thread specified in
 * the scheduler
 */
bool check_current(void);

/* Wakes up the threads waiting for the running condition and blocks the current one */
void run(bool blocked);

/* Sets a thread that is not waiting as the current one
 * Also resets the current thread's time
 */
void no_time(void);

/* Resets the current thread's time, then changes it */
void waiting(void);

/* Sets the next non-waiting thread with the highest priority in the queue as the current thread,
 * or resets the current one's time, if no other threads are found
 */
void next(void);

/* Sets the first current thread */
void first(void);

/* The scheduling function */
void schedule(bool blocked);

/* A thread's start routine
 * p holds the parameters
 */
void *start_thread(void *p);

/* Checks if a thread's tid was initialized by the scheduler */
bool find_thread(tid_t tid);

#endif
