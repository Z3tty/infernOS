#include "scheduler.h"
#include "screen.h"
#include "thread.h"
#include "util.h"

/* Threads to test barrier */

enum
{
	ITERATIONS = 20000,
};

volatile int b1v = 0;
volatile int b2v = 0;
volatile int b3v = 0;
volatile int barrier_initialized = 0;
barrier_t bar;

void barrier1(void) {
	int i, j;

	barrier_init(&bar, 3);
	barrier_initialized = 1;
	print_str(BARRIER_LINE, BARRIER_COL, "Barrier");
	print_str(BARRIER_LINE + 1, BARRIER_COL, "Running");

	for (i = 0; i < ITERATIONS; i++) {
		b1v++;
		print_str(BARRIER_LINE + 1, BARRIER_COL, "       ");
		print_int(BARRIER_LINE + 1, BARRIER_COL, b1v);
		barrier_wait(&bar);
		for (j = 0; j < b1v; j++)
			;
		ASSERT(b1v == b2v);
		ASSERT(b2v == b3v);
		barrier_wait(&bar);
	}

	ASSERT(b1v == ITERATIONS);
	print_str(BARRIER_LINE, BARRIER_COL, "Barrier");
	print_str(BARRIER_LINE + 1, BARRIER_COL, "1:OK   ");
	exit();
}

void barrier2(void) {
	int i, j;

	while (barrier_initialized != 1)
		; /* Spin */

	for (i = 0; i < ITERATIONS; i++) {
		b2v++;
		barrier_wait(&bar);
		for (j = 0; j < 2 * b2v; j++)
			;
		ASSERT(b1v == b2v);
		ASSERT(b2v == b3v);
		barrier_wait(&bar);
	}

	ASSERT(b2v == ITERATIONS);
	print_str(BARRIER_LINE, BARRIER_COL, "Barrier");
	print_str(BARRIER_LINE + 1, BARRIER_COL + 7, "2:OK, ");
	exit();
}

void barrier3(void) {
	int i, j;

	while (barrier_initialized != 1)
		; /* Spin */

	for (i = 0; i < ITERATIONS; i++) {
		b3v++;
		barrier_wait(&bar);
		for (j = 0; j < 3 * b3v; j++)
			;
		ASSERT(b1v == b2v);
		ASSERT(b2v == b3v);
		barrier_wait(&bar);
	}

	ASSERT(b3v == ITERATIONS);
	print_str(BARRIER_LINE, BARRIER_COL, "Barrier");
	print_str(BARRIER_LINE + 1, BARRIER_COL + 14, "3:OK");
	exit();
}
