/*
 * Implementation of spinlocks, locks and condition variables
 */

#include "common.h"
#include "scheduler.h"
#include "thread.h"
#include "util.h"

/* spinlock functions */

/*
 * Atomic test-and-set operation on a spinlock flag
 *
 * Sets the lock to true/1, and then tests the previous value.
 * If the value was already true/1, then the lock was already held by another
 * thread. If the value was false/0, then it was this thread that acquired the
 * lock.
 *
 * This is implemented with C's built-in atomic test-and-set operation, which
 * has been part of the C standard since C11. This will compile to an atomic
 * instrunction (such as XCHG on x86). It also tells the optimizer that it must
 * not rearrange statements in a way that would change the behavior of the lock.
 *
 * For gory details on C11 atomics and memory synchronization, see:
 *
 * GCC Documentation: __atomic Builtins
 *  https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html
 *
 * GCC Wiki: Atomic/GCCMM/AtomicSync
 *  https://gcc.gnu.org/wiki/Atomic/GCCMM/AtomicSync
 */
static spinlock_t test_and_set(spinlock_t *addr) {
	return __atomic_test_and_set(addr, __ATOMIC_ACQUIRE);
}

/*
 * Atomic clear operation on a spinlock flag
 *
 * This is the compliment to test_and_set. Like test_and_set, it is implemented
 * with C's built-in atomic operations, to prevent unwanted compiler
 * optimizations that might break the lock.
 */
static void atomic_clear(spinlock_t *addr) {
	__atomic_clear(addr, __ATOMIC_RELEASE);
}

void spinlock_init(spinlock_t *s) {
	*s = UNLOCKED;
}

/*
 * Wait until lock is free, then acquire lock. Note that this isn't
 * strictly a spinlock, as it yields while waiting to acquire the
 * lock. This implementation is thus more "generous" towards
 * other processes, but might also generate higher latencies on
 * acquiring the lock.
 */
void spinlock_acquire(spinlock_t *s) {
	while (test_and_set(s) != UNLOCKED)
		yield();
}

/*
 * Release a spinlock
 *
 * No waiting is required since this thread holds the lock.
 * We still use the atomic operations though, to constrain the optimizer.
 */
void spinlock_release(spinlock_t *s) {
	ASSERT2(*s == LOCKED, "spinlock should be locked");
	atomic_clear(s);
}

/* lock functions  */

void lock_init(lock_t *l) {
	/*
	 * no need for critical section, it is callers responsibility
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
	} else {
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
