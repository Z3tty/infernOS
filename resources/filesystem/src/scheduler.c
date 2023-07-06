#include "interrupt.h"
#include "kernel.h"
#include "scheduler.h"
#include "thread.h"
#include "time.h"
#include "util.h"

/* Insert 'job' in the ready queue. 'job' is inserted after 'q' */
static void insert_job(pcb_t *job, pcb_t *q);
/* Remove 'job' from the ready queue */
static void remove_job(pcb_t *job);

/* Call scheduler to run the 'next' process */
void yield(void) {
	enter_critical();
	current_running->yield_count++;
	scheduler_entry();
	leave_critical();
}

/*
 * The scheduler must be called within a critical section since it
 * changes global state, and since dispatch() needs to be called
 * within a critical section.  Disable_count for current_running is
 * checked to see that it's != 0. The scheduler also handles saving
 * the current interrupt controller mask (which is later restored in
 * setup_current_running()).
 */
void scheduler(void) {
	unsigned long long t;

	/*
	 * Save hardware interrupt mask in the pcb struct. The mask
	 * will be restored in setup_current_running()
	 */
	current_running->int_controller_mask = ((uint16_t)inb(0x21)) | (((uint16_t)inb(0xa1)) << 8);

	ASSERT(current_running->disable_count != 0);

	do {
		switch (current_running->status) {
		case SLEEPING:
			t = get_timer();
			if (current_running->wakeup_time < t)
				current_running->status = RUNNING;
			/* FALLTHROUGH */
		case RUNNING:
			/* pick the next job to run */
			current_running = current_running->next;
			break;
		case BLOCKED:
			/* if no more jobs, halt */
			if (current_running->next == current_running) {
				HALT("No more jobs.");
			}

			current_running = current_running->next;
			/* Remove the job from the ready queue */
			remove_job(current_running->previous);
			break;
		case EXITED:
			/* if no more jobs, loop forever */
			if (current_running->next == current_running) {
				HALT("No more jobs.");
			}

			current_running = current_running->next;
			/*
			 * Remove pcb from the ready queue and insert
			 * it into the free_pcb queue
			 */
			free_pcb(current_running->previous);
			break;
		default:
			HALT("Invalid job status.");
			break;
		}
	} while (current_running->status != RUNNING);

	/* .. and run it */
	dispatch();
}

/* Helper function for dispatch() */
void setup_current_running(void) {
	/* Restore harware interrupt mask */
	outb(0x21, (uint8_t)(current_running->int_controller_mask & 0xff));
	outb(0xa1, (uint8_t)(current_running->int_controller_mask >> 8));

	/* Load pointer to the page directory of current_running into CR3 */
	select_page_directory();
	reset_timer();

	if (!current_running->is_thread) { /* process */
		/*
		 * Setup the kernel stack that will be loaded when the
		 * process is interrupted and enters the kernel.
		 */
		tss.esp_0 = (uint32_t)current_running->base_kernel_stack;
		tss.ss_0 = KERNEL_DS;
	}
}

/*
 * Remove the current_running process from the linked list so it will
 * not be scheduled in the future
 */
void exit(void) {
	enter_critical();
	current_running->status = EXITED;
	/* Removes job from ready queue, and dispatchs next job to run */
	scheduler_entry();
	/*
	 * No leave_critical() needed, as we never return here. (The
	 * process is exiting!)
	 */
}

/*
 * q is a pointer to the waiting list where current_running should be
 * inserted.
 */
void block(pcb_t **q, spinlock_t *spinlock) {
	pcb_t *tmp;

	enter_critical();

	if (spinlock)
		spinlock_release(spinlock);

	/* mark the job as blocked */
	current_running->status = BLOCKED;

	/* Insert into waiting list */
	tmp = *q;
	(*q) = current_running;
	current_running->next_blocked = tmp;

	/* remove job from ready queue, pick next job to run and dispatch it */
	scheduler_entry();
	leave_critical();
}

/*
 * Unblocks the first process in the waiting queue (q), (*q) points to
 * the last process.
 */
void unblock(pcb_t **q) {
	pcb_t *new, *tmp;

	enter_critical();
	ASSERT((*q) != NULL);

	if ((*q)->next_blocked == NULL) {
		new = (*q);
		(*q) = NULL;
	}
	/* (*q) not only pcb in queue */
	else {
		for (tmp = *q; tmp->next_blocked->next_blocked != NULL; tmp = tmp->next_blocked)
			/* do nothing */;

		new = tmp->next_blocked;
		/* new = last pcb in waiting queue */
		tmp->next_blocked = NULL;
	}

	new->status = RUNNING;
	/* Add the process to active process queue */
	insert_job(new, current_running);
	leave_critical();
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

/* Insert 'job' in queue the ready queue 'job' is inserted before q */
static void insert_job(pcb_t *job, pcb_t *q) {
	job->next = q;
	job->previous = q->previous;
	q->previous->next = job;
	q->previous = job;
}

/* Remove 'job' from the ready queue */
static void remove_job(pcb_t *job) {
	job->previous->next = job->next;
	job->next->previous = job->previous;
}
