/*  scheduler.c
 */
#include "common.h"
#include "kernel.h"
#include "scheduler.h"
#include "util.h"

// Call scheduler to run the 'next' process
void yield(void) {
}

/* The scheduler picks the next job to run, and removes blocked and exited
 * processes from the ready queue, before it calls dispatch to start the
 * picked process.
 */
void scheduler(void) {
}

/* dispatch() does not restore gpr's it just pops down the kernel_stack,
 * and returns to whatever called scheduler (which happens to be scheduler_entry,
 * in entry.S).
 */
void dispatch(void) {
}

/* Remove the current_running process from the linked list so it
 * will not be scheduled in the future
 */
void exit(void) {
}

/* 'q' is a pointer to the waiting list where current_running should be
 * inserted
 */
void block(pcb_t **q) {
}

/* Must be called within a critical section.
 * Unblocks the first process in the waiting queue (q), (*q) points to the
 * last process.
 */
void unblock(pcb_t **q) {
}
