/*
 * Adapted from linux kernel sources:
 * /usr/src/linux/arch/i386/kernel/time.c
 */

#ifndef TIME_H
#define TIME_H

#include "common.h"

enum
{
	HZ = 100,
	CLOCK_TICK_RATE = 1193180, /* Underlying HZ */
	/* LATCH is used in the interval timer and ftape setup. */
	LATCH = ((CLOCK_TICK_RATE + HZ / 2) / HZ), /* For divider */
	CALIBRATE_LATCH = (5 * LATCH),
	CALIBRATE_TIME = (5 * 1000020 / HZ)
};

/* from #include <linux/timex.h>: */
#define rdtsc(low, high) __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high))

/* Exported variables */
extern uint32_t cpu_mhz;

/* Exported functions (called once by the kernel in _start()) */
void time_init(void);

#endif /* !TIME_H */
