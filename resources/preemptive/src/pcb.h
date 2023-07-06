#ifndef PCB_H
#define PCB_H

#include "pcb_status.h"

/*
 * The process control block is used for storing various information about
 * a thread or process
 */
typedef struct pcb {
	uint32_t pid;        /* Process id of this process */
	uint32_t is_thread;  /* Thread or process */
	uint32_t user_stack; /* Pointer to base of the user stack */
	/*
	 * Used to set up kernel stack, and temporary save esp in
	 * scheduler()/dispatch()
	 */
	uint32_t kernel_stack;
	/*
	 * 0: process in user mode
	 * 1: process/thread in kernel mode
	 * 2: process/thread that was in kernel mode when it got interrupted
	 *    (and is still in kernel mode).
	 *
	 * It is used to decide if we should switch to/from the kernel_stack,
	 * in switch_to_kernel/user_stack
	 */
	uint32_t nested_count; /* Number of nested interrupts */
	uint32_t start_pc;     /* Start address of a process or thread */
	uint32_t priority;     /* This process' priority */
	uint32_t status;       /* FIRST_TIME, READY, BLOCKED or EXITED */
	struct pcb *next;      /* Used when job is in the ready queue */
	struct pcb *previous;
} pcb_t;

#endif /* !PCB_H */
