/*
 * Implementation of spinlocks, locks and condition variables
 */

#include "common.h"
#include "scheduler.h"
#include "thread.h"
#include "util.h"

/* spinlock functions */

/*
 * xchg exchanges the given value with the value in *addr. This means
 * that this function will write LOCKED to *addr and return the value
 * that was in *addr.
 *
 * It is done atomically, so if the return value from test_and_set
 * is UNLOCKED, the caller succeeded in getting the lock.
 * If the return value is LOCKED, then somebody else had the lock and
 * you have to try again.
 */
static int test_and_set(int *addr) {
	int val = LOCKED;

	asm volatile("xchg (%%eax), %%ebx" : "=b"(val) : "0"(val), "a"(addr));
	return val;
}

void spinlock_init(int *s) {
	*s = UNLOCKED;
}

/*
 * Wait until lock is free, then acquire lock. Note that this isn't
 * strictly a spinlock, as it yields while waiting to acquire the
 * lock. This implementation is thus more "generous" towards
 * other processes, but might also generate higher latencies on
 * acquiring the lock.
 */
void spinlock_acquire(int *s) {
	while (test_and_set(s) != UNLOCKED)
		yield();
}

/*
 * Releasing a spinlock is pretty easy, since it only consists of
 * writing UNLOCKED to the address. This does not have to be done
 * atomically.
 */
void spinlock_release(int *s) {
	ASSERT2(*s == LOCKED, "spinlock should be locked");
	*s = UNLOCKED;
}

/* lock functions  */

void lock_init(lock_t *l) {
	/*
	 * nno need for critical section, it is callers responsibility
	 * to make sure that locks are initialized only once
	 */
	spinlock_init(&l->spinlock);
	l->status = UNLOCKED;
	l->waiting = NULL;
}

void lock_acquire(lock_t *l) {
	spinlock_acquire(&l->spinlock);
	if (l->status == UNLOCKED) {
		l->status = LOCKED;
		spinlock_release(&l->spinlock);
		return;
	}
	else {
		block(&l->waiting, &l->spinlock);
	}
}

void lock_release(lock_t *l) {
	spinlock_acquire(&l->spinlock);
	if (l->waiting == NULL)
		l->status = UNLOCKED;
	else
		unblock(&l->waiting);

	spinlock_release(&l->spinlock);
}

/* condition functions */

void condition_init(condition_t *c) {
	spinlock_init(&c->spinlock);
	c->waiting = NULL;
}

/*
 * unlock m and block the thread (enqued on c), when unblocked acquire
 * lock m
 */
void condition_wait(lock_t *m, condition_t *c) {
	spinlock_acquire(&c->spinlock);
	lock_release(m);
	block(&c->waiting, &c->spinlock);
	lock_acquire(m);
}

/* unblock first thread enqued on c */
void condition_signal(condition_t *c) {
	spinlock_acquire(&c->spinlock);
	if (c->waiting != NULL)
		unblock(&c->waiting);
	spinlock_release(&c->spinlock);
}

/* unblock all threads enqued on c */
void condition_broadcast(condition_t *c) {
	spinlock_acquire(&c->spinlock);
	while (c->waiting != NULL) {
		unblock(&c->waiting);
	}
	spinlock_release(&c->spinlock);
}
