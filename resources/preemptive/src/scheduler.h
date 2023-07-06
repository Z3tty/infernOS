#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"
#include "dispatch.h"
#include "kernel.h"

/* Calls scheduler to run the 'next' process */
void yield(void);

/* Save context and kernel stack, before calling scheduler (in entry.S) */
void scheduler_entry(void);

/*
 * Figure out which process should be next to run, and remove current process
 * from the ready queue if the process has exited or is blocked (ie,
 * cr->status == BLOCKED or cr->status == EXITED).
 */
void scheduler(void);
/*
 * Set status = EXITED and call scheduler_entry() which will remove the
 * job from the ready queue and pick next process to run.
 */
void exit(void);
/* Return process id of current process */
int getpid(void);
/* Get and set current process' priority */
int getpriority(void);
void setpriority(int);
/* Returns the current value of cpu_mhz */
int cpuspeed(void);
/* Remove current running from ready queue and insert it into 'q'. */
void block(pcb_t **q);
/* Move first process in 'q' into the ready queue */
void unblock(pcb_t **q);
/* Switch to kernel stack if current_running->nested_count == 0 */
void switch_to_kernel_stack(void);
/* Switch to user stack if current_running->nested_count == 0 */
void switch_to_user_stack(void);

#endif /* !SCHEDULER_H */
