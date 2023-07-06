/*
 * Implementation of spinlocks, locks and condition variables
 */
#ifndef THREAD_H
#define THREAD_H

#include "kernel.h"

enum { LOCKED = 1, UNLOCKED = 0 };

/* Typedefs */

typedef uint8_t spinlock_t;

typedef struct {
	pcb_t *waiting; /* waiting queue */
	int status;     /* locked or unlocked */
	spinlock_t spinlock;
} lock_t;

typedef struct {
	spinlock_t spinlock;
	pcb_t *waiting; /* waiting queue */
} condition_t;

/* Spinlock functions */
void spinlock_init(spinlock_t *s);
void spinlock_acquire(spinlock_t *s);
void spinlock_release(spinlock_t *s);

/* Lock functions */
void lock_init(lock_t *);
void lock_acquire(lock_t *);
void lock_release(lock_t *);

/* Initialize c */
void condition_init(condition_t *c);

/* Unlock m and block on condition c, when unblocked acquire lock m */
void condition_wait(lock_t *m, condition_t *c);

/* Unblock first thread enqued on c */
void condition_signal(condition_t *c);

/* Unblock all threads enqued on c */
void condition_broadcast(condition_t *c);

#endif /* !THREAD_H */
