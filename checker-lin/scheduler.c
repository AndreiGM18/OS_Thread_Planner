// SPDX-License-Identifier: EUPL-1.2
/* Copyright Mitran Andrei-Gabriel 2022 */

#include <stdio.h>
#include <string.h>

#include "queue.h"
#include "thread.h"
#include "utils.h"
#include "scheduler.h"

/* The scheduler */
static struct scheduler_t *scheduler;

/* Variable that is used to tell if the scheduler was initialized or not */
static bool init;

DECL_PREFIX int so_init(uint time_quantum, uint io)
{
	if (init == TRUE || time_quantum == NO_TIME || io > SO_MAX_NUM_EVENTS)
		return ERROR;

	scheduler = (struct scheduler_t *)malloc(sizeof(struct scheduler_t));
	DIE(!scheduler, "scheduler malloc() failed");

	/* Creating the queues */
	scheduler->q = create(sizeof(struct thread_t));
	scheduler->all_threads = create(sizeof(tid_t));

	/* Initializing the mutex and the conditions */
	DIE(pthread_mutex_init(&scheduler->mutex, NULL), "mutex init failed");
	DIE(pthread_cond_init(&scheduler->ready, NULL), "ready cond init failed");
	DIE(pthread_cond_init(&scheduler->running, NULL), "running cond init failed");

	/* Initializing the rest of the fields */
	scheduler->io = io;
	scheduler->time_quantum = time_quantum;
	scheduler->is_ready = scheduler->is_finished = FALSE;
	scheduler->current = NULL;
	scheduler->current_tid = EMPTY;

	/* Setting the scheduler as having been initialized */
	init = TRUE;

	return SUCCESS;
}

bool check_current(void)
{
	tid_t current_tid = (tid_t)pthread_self();

	DIE(current_tid < 0, "invalid tid");

	return scheduler->current_tid == current_tid;
}

void run(bool blocked)
{
	/* All threads that are waiting for the running condition are woken up */
	DIE(pthread_cond_broadcast(&scheduler->running), "running broadcast failed");
	DIE(pthread_mutex_unlock(&scheduler->mutex), "mutex unlock failed");

	/* If the current thread is not the one specified in the scheduler and the blocked condition
	 * is TRUE
	 */
	if (blocked == TRUE && check_current() == FALSE) {
		/* Make the current thread wait for the running condition */
		DIE(pthread_mutex_lock(&scheduler->mutex), "mutex lock failed");
		while (check_current() == FALSE)
			DIE(pthread_cond_wait(&scheduler->running, &scheduler->mutex), "running wait failed");
		DIE(pthread_mutex_unlock(&scheduler->mutex), "mutex unlock failed");
	}
}

void no_time(void)
{
	/* The current thread's time is reset */
	scheduler->current->time = scheduler->time_quantum;

	struct thread_t *thread = front_not_waiting(scheduler->q);

	/* If a thread with a greater priority is found, changes the current one
	 * If not, the threads are reordered, and then the current thread is changed
	 */
	if (thread->priority > scheduler->current->priority) {
		scheduler->current_tid = thread->tid;
		scheduler->current = thread;
	} else {
		reorder(scheduler->q);

		thread = front_not_waiting(scheduler->q);

		if (thread) {
			scheduler->current_tid = thread->tid;
			scheduler->current = thread;
		}
	}
}

void waiting(void)
{
	scheduler->current->time = scheduler->time_quantum;

	struct thread_t *thread = front_not_waiting(scheduler->q);

	scheduler->current_tid = thread->tid;
	scheduler->current = thread;
}

void next(void)
{
	struct thread_t *thread = front_not_waiting(scheduler->q);

	if (thread) {
		/* A thread was found, so changes the current thread in the scheduler */
		scheduler->current_tid = thread->tid;
		scheduler->current = thread;

	} else
		/* A thread was not found, resets the current one's time */
		scheduler->current->time = scheduler->time_quantum;
}

void first(void)
{
	struct thread_t *thread = front_not_waiting(scheduler->q);

	/* The thread becomes the current one in the scheduler */
	scheduler->current_tid = thread->tid;
	scheduler->current = thread;

	/* The thread is woken up */
	DIE(pthread_cond_broadcast(&scheduler->running), "running broadcast failed");
	DIE(pthread_mutex_unlock(&scheduler->mutex), "mutex unlock failed");
}

void schedule(bool blocked)
{
	DIE(pthread_mutex_lock(&scheduler->mutex), "mutex lock failed");

	if (is_empty(scheduler->q) == TRUE)
		return;

	if (scheduler->current_tid == EMPTY) {
		/* The first ever thread */
		first();

		return;
	}

	if (scheduler->is_finished == TRUE) {
		/* The current thread has simply finished, calls next() */
		scheduler->is_finished = FALSE;

		next();
	} else if (scheduler->current->is_waiting == TRUE) {
		/* The current thread is waiting */
		waiting();
	} else if (scheduler->current->time == NO_TIME) {
		/* The current thread has run out of time */
		no_time();
	} else {
		next();
	}

	run(blocked);
}

void *start_thread(void *p)
{
	struct parameter_t *parameters = (struct parameter_t *)p;

	uint priority = parameters->priority;
	so_handler *handler_func = parameters->func;

	/* Signals to the parent that the thread is ready for scheduling */
	DIE(pthread_mutex_lock(&scheduler->mutex), "mutex lock failed");
	scheduler->is_ready = TRUE;
	DIE(pthread_cond_signal(&scheduler->ready), "ready signal failed");
	DIE(pthread_mutex_unlock(&scheduler->mutex), "mutex unlock failed");

	/* Wait for the thread to be scheduled */
	DIE(pthread_mutex_lock(&scheduler->mutex), "mutex lock failed");
	while (check_current() == FALSE)
		DIE(pthread_cond_wait(&scheduler->running, &scheduler->mutex),
			"running wait failed");
	DIE(pthread_mutex_unlock(&scheduler->mutex), "mutex unlock failed");

	/* Runs the handler function */
	handler_func(priority);

	/* The thread has finished */
	scheduler->is_finished = TRUE;

	/* Remove the thread from the queue */
	remove_thread(scheduler->q, scheduler->current);

	schedule(FALSE);

	return NULL;
}

bool find_thread(tid_t tid)
{
	struct node_t *it = scheduler->all_threads->front;

	/* Checks all nodes */
	while (it) {
		if (*((tid_t *)it->data) == tid)
			return TRUE;

		it = it->next;
	}

	return FALSE;
}

DECL_PREFIX tid_t so_fork(so_handler *func, uint priority)
{
	if (func == NULL || priority > SO_MAX_PRIO)
		return INVALID_TID;

	/* A new thread */
	struct thread_t new;

	new.priority = priority;
	new.func = func;
	new.time = scheduler->time_quantum;
	new.is_waiting = FALSE;
	new.io = EMPTY;

	tid_t tid;

	/* The start routine's parameters */
	struct parameter_t parameters;

	parameters.priority = priority;
	parameters.func = func;

	/* Creates a thread */
	DIE(pthread_create(&tid, NULL, &start_thread, (void *) &parameters),
		"thread creation failed");

	/* Sets the tid */
	new.tid = tid;

	/* Adds the thread in this queue */
	enqueue(scheduler->q, &new, new.priority);

	/* Adds the tid in this queue */
	enqueue(scheduler->all_threads, &tid, BASIC_PRIO);

	/* Waits for the thread to be ready */
	DIE(pthread_mutex_lock(&scheduler->mutex), "mutex lock failed");
	while (scheduler->is_ready == FALSE)
		DIE(pthread_cond_wait(&scheduler->ready, &scheduler->mutex), "ready wait failed");
	scheduler->is_ready = FALSE;
	DIE(pthread_mutex_unlock(&scheduler->mutex), "mutex unlock failed");

	/* If the actual current thread's tid was found,
	 * the current thread's time is decreased
	 */
	if (find_thread((tid_t)pthread_self()) == TRUE)
		--scheduler->current->time;

	schedule(TRUE);

	return tid;
}

DECL_PREFIX int so_wait(uint io)
{
	/* Decreases the current thread's time */
	--scheduler->current->time;

	if (io >= scheduler->io) {
		schedule(TRUE);
		return ERROR;
	}

	/* Sets the thread as waiting for io */
	scheduler->current->is_waiting = TRUE;
	scheduler->current->io = io;

	schedule(TRUE);

	return SUCCESS;
}

DECL_PREFIX int so_signal(uint io)
{
	/* Decreases the current thread's time */
	--scheduler->current->time;

	if (io >= scheduler->io) {
		schedule(TRUE);

		return ERROR;
	}

	int threads_woken = wake_up(scheduler->q, io);

	schedule(TRUE);

	return threads_woken;
}

DECL_PREFIX void so_exec(void)
{
	/* Decreases the current thread's time */
	--scheduler->current->time;

	schedule(TRUE);
}

DECL_PREFIX void so_end(void)
{
	if (init == FALSE)
		return;

	struct node_t *it = scheduler->all_threads->front;

	/* Join all threads */
	while (it) {
		pthread_join(*((tid_t *)it->data), NULL);
		it = it->next;
	}

	/* Free the queues */
	free_q(scheduler->q, free);
	free_q(scheduler->all_threads, free);

	/* Preemptively lock and unlock the mutex, then destroys the mutex */
	pthread_mutex_trylock(&scheduler->mutex);
	pthread_mutex_unlock(&scheduler->mutex);
	DIE(pthread_mutex_destroy(&scheduler->mutex), "mutex destroy failed");

	/* Destroys the conditions */
	DIE(pthread_cond_destroy(&scheduler->ready), "ready destroyed failed");
	DIE(pthread_cond_destroy(&scheduler->running), "running destroy failed");

	/* Frees the scheduler struct */
	free(scheduler);

	/* The init condition is set to FALSE */
	init = FALSE;
}
