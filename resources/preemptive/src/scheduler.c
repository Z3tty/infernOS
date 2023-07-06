#include "common.h"
#include "dispatch.h"
#include "interrupt.h"
#include "kernel.h"
#include "scheduler.h"
#include "time.h"
#include "util.h"

/* Remove 'job' from the ready queue */
static void remove_job(pcb_t *job);

/* Call scheduler to run the 'next' process */
void yield(void) {
	enter_critical();
	yield_count++;
	scheduler_entry();
	leave_critical();
}

/*
 * The scheduler must be called within a critical section since it
 * changes global state, and since dispatch() needs to be called
 * within a critical section.  Disable_count is checked to see that
 * it's != 0.
 */
void scheduler(void) {
	ASSERT(disable_count != 0);

	switch (current_running->status) {
	case STATUS_READY:
		/* pick the next job to run */
		current_running = current_running->next;
		break;
	case STATUS_BLOCKED:
	case STATUS_EXITED:
		/* if no more jobs halt */
		if (current_running->next == current_running) {
			HALT("No more jobs.");
		}

		current_running = current_running->next;
		/* remove the job from the job list */
		remove_job(current_running->previous);
		break;
	default:
		HALT("Invalid job status.");
		break;
	}

	/* .. and run it */
	dispatch();
}

/*
 * Remove the current_running process from the linked list so it will
 * not be scheduled in the future
 */
void exit(void) {
	enter_critical();
	current_running->status = STATUS_EXITED;
	/* Removes job from ready queue, and dispatches next job to run */
	scheduler_entry();
	/*
	 * No leave_critical() needed, as we never return here. (The
	 * process is exiting!)
	 */
}

/*
 * Must be called within a critical section.  'q' is a pointer to the
 * waiting list where current_running should be inserted
 */
void block(pcb_t **q) {
	pcb_t *p;

	ASSERT(disable_count != 0);

	/*
	 * Mark the job as blocked, scheduler will remove it from the ready
	 * queue.
	 */
	current_running->status = STATUS_BLOCKED;

	/* put the job on the waiting queue */
	if (*q == NULL) {
		*q = current_running;
	}
	else {
		p = *q;

		while (p->next != NULL) {
			p = p->next;
		}
		p->next = current_running;
	}

	/* pick next job to run */
	scheduler_entry();
}

/*
 * Must be called within a critical section.  Unblocks the first
 * process in the waiting queue (q), (*q) points to the last process.
 */
void unblock(pcb_t **q) {
	pcb_t *job;

	ASSERT(disable_count != 0);

	/* remove the job from the waiting queue */
	job = *q;
	(*q) = (*q)->next;

	/* put the job back on the job list */
	job->status = STATUS_READY;
	job->next = current_running->next;
	job->previous = current_running;
	current_running->next->previous = job;
	current_running->next = job;
}

int getpid(void) {
	return current_running->pid;
}

int getpriority(void) {
	return current_running->priority;
}

void setpriority(int p) {
	current_running->priority = p;
}

int cpuspeed(void) {
	return cpu_mhz;
}

/* Remove 'job' from the ready queue */
static void remove_job(pcb_t *job) {
	job->previous->next = job->next;
	job->next->previous = job->previous;
	job->next = NULL;
	job->previous = NULL;
}
