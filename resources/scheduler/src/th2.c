/* Simple programs to check the functionality of lock_acquire(),
 * and lock_release().
 */

#include "common.h"
#include "lock.h"
#include "scheduler.h"
#include "th.h"
#include "util.h"

#define RUNNING 0
#define OK 1
#define FAILED 2
#define DONE 3
#define PRINT_LINE 18

int shared_var = 0;
int exit_count = 0;               /* number of threads that have exited */
static volatile int init = FALSE; /* is lock l initialized */
lock_t l;

static void print_counter(int thread, int status);

void thread2(void) {
	int tmp, i;

	/* This thread initializes the lock, the other thread waits for init to
	 * become true */
	if (!init) {
		lock_init(&l);
		init = TRUE;
	}
	/* Do some testing */
	for (i = 0; i < 100; i++) {
		/* Acquire lock before using shared_var */
		lock_acquire(&l);

		tmp = shared_var;
		/* Give thread3 a chance to run */
		if (i % 13 == 0) {
			/* Yielding inside the critical section  might trigger some
			 * errors in bad implementations of the locks. */
			yield();
		}
		/* shared_var should be equal to tmp. If the synchronization
		 *  primitives are not working correctly then shared_var > tmp,
		 *  because thread3 has increased shared_var.
		 */
		shared_var = tmp + 1;

		/* Done using shared_var */
		lock_release(&l);
		print_counter(2, RUNNING);
		/* Give thread3 a chance to run */
		yield();
	}

	exit_count++;

	/* When both threads have finished, print the result */
	if (exit_count == 2) {
		print_counter(2, (shared_var == 200) ? OK : FAILED);
	}
	else {
		print_counter(2, DONE);
	}
	exit();
}

void thread3(void) {
	int tmp, i;

	/* Wait until thread2 initializes the lock */
	if (!init) {
		lock_init(&l);
		init = TRUE;
	}
	/* Do some testing */
	for (i = 0; i < 100; i++) {
		/* Acquire lock before using shared_var */
		lock_acquire(&l);
		tmp = shared_var;
		/* give thread2 a chance to run */
		if (i % 17 == 0) {
			/* Yielding inside the critical section  might trigger some
			 * errors in bad implementations of the locks. */
			yield();
		}
		/* shared_var should be equal to tmp. If the synchronization
		 *  primitives are not working correctly then shared_var > tmp,
		 *  because thread2 has increased shared_var.
		 */
		shared_var = tmp + 1;
		/* Done using shared_var */
		lock_release(&l);
		print_counter(3, RUNNING);
		/* Give thread2 a chance to run */
		yield();
	}
	exit_count++;

	/* When both threads have finished, print the result */
	if (exit_count == 2) {
		print_counter(3, (shared_var == 200) ? OK : FAILED);
	}
	else {
		print_counter(3, DONE);
	}
	exit();
}

static void print_counter(int thread, int status) {
	static int counter2 = 0, counter3 = 0;
	int line = PRINT_LINE + thread;

	if (thread == 2) {
		print_str(PRINT_LINE + 2, 0, "Thread  2 (lock)      : ");
	}
	else {
		print_str(PRINT_LINE + 3, 0, "Thread  3 (lock)      : ");
	}
	delay(DELAY_VAL);
	switch (status) {
	case RUNNING:
		if (thread == 2) {
			print_int(PRINT_LINE + 2, 25, counter2++);
		}
		else {
			print_int(PRINT_LINE + 3, 25, counter3++);
		}
		break;
	case DONE:
		print_str(line, 25, "Exited");
		break;
	case OK:
		print_str(line, 25, "Passed");
		break;
	default:
		print_str(line, 25, "Failed");
		break;
	}
}
