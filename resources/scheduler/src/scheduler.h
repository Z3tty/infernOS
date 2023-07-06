/*  scheduler.h
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

// Includes
#include "common.h"
#include "kernel.h"

// Constants
enum
{
	STATUS_FIRST_TIME = 0,
	STATUS_READY,
	STATUS_BLOCKED,
	STATUS_EXITED
};

// Prototypes

// Calls scheduler to run the 'next' process
void yield(void);

// Save context and kernel stack, before calling scheduler (in entry.S)
void scheduler_entry(void);

/* Figure out which process should be next to run, and remove current process
 * from the ready queue if the process has exited or is blocked (ie,
 *  cr->status == STATUS_BLOCKED or cr->status == STATUS_EXITED).
 */
void scheduler(void);
// Dispatch next process
void dispatch(void);

/* Set status = STATUS_EXITED and call scheduler_entry() which will remove the
 *  job from the ready queue and pick next process to run.
 */
void exit(void);

/* Remove current running from ready queue and insert it into 'q'.
 */
void block(pcb_t **q);

// Move first process in 'q' into the ready queue
void unblock(pcb_t **q);
#endif
