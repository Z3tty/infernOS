#ifndef LOCK_H
#define LOCK_H

#include "kernel.h"

enum
{
	LOCKED = 1,
	UNLOCKED = 0
};

typedef struct {
    // TODO
} lock_t;

void lock_init(lock_t *);
void lock_acquire(lock_t *);
void lock_release(lock_t *);

#endif
