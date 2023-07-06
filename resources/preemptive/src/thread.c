/*
 * Implementation of locks and condition variables
 */

#include "common.h"
#include "interrupt.h"
#include "scheduler.h"
#include "thread.h"
#include "util.h"

void lock_init(lock_t *l) {
	/*
	 * no need for critical section, it is callers responsibility to
	 * make sure that locks are initialized only once
	 */
	l->status = UNLOCKED;
	l->waiting = NULL;
}

/* Acquire lock without critical section (called within critical section) */
static void lock_acquire_helper(lock_t *l) {
	if(l){ /* compiler go :shushing_face: */
	}
}

void lock_acquire(lock_t *l) {
	if(l){ /* compiler go :shushing_face: */
	}
	/* another silence placeholder */
	lock_acquire_helper(l);
}

void lock_release(lock_t *l) {
	if(l){ /* compiler go :shushing_face: */
	}
}

/* condition functions */

void condition_init(condition_t *c) {
	if(c){ /* compiler go :shushing_face: */
	}
}

/*
 * unlock m and block the thread (enqued on c), when unblocked acquire
 * lock m
 */
void condition_wait(lock_t *m, condition_t *c) {
	if(m && c){ /* compiler go :shushing_face: */
	}
}

/* unblock first thread enqued on c */
void condition_signal(condition_t *c) {
	if(c){ /* compiler go :shushing_face: */
	}
}

/* unblock all threads enqued on c */
void condition_broadcast(condition_t *c) {
	if(c){ /* compiler go :shushing_face: */
	}
}

/* Semaphore functions. */
void semaphore_init(semaphore_t *s, int value) {
	if(s && value){ /* compiler go :shushing_face: */
	}
}

void semaphore_up(semaphore_t *s) {
	if(s){ /* compiler go :shushing_face: */
	}
}

void semaphore_down(semaphore_t *s) {
	if(s){ /* compiler go :shushing_face: */
	}
}

/*
 * Barrier functions
 */

/* n = number of threads that waits at the barrier */
void barrier_init(barrier_t *b, int n) {
	if(b && n){ /* compiler go :shushing_face: */
	}
}

/* Wait at barrier until all n threads reach it */
void barrier_wait(barrier_t *b) {
	if(b){ /* compiler go :shushing_face: */
	}
}
