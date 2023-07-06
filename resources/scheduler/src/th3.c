/*
 * Monte Carlo Pi.
 *
 * The Monte Carlo method of estimating pi uses a unitary circle,
 * inscribed inside a square. The application repeatedly picks a random
 * point within the square, the fraction of the points that are inside
 * the unitary circle then represents pi/4.
 *
 * Implementation trades Pi accuracy and performance for determenism.
 *
 * Excpected behavior:
 * - Accuracy of printed results increase.
 * - Final result is always the same.
 *
 * Note! Threads have not been debugged. The file may be changed.
 */

#include "lock.h"
#include "scheduler.h"
#include "util.h"

enum
{
	WORKERS = 4,     // Number of worker threads
	DARTS = 1000,    //	2000000, // Number of random points
	RES_GRAN = 1000, // Output granularity
	MCPI_Y = 22,     // Line where results are printed
	MCPI_X0 = 25,
	MCPI_X1 = 35,
	MCPI_X2 = 45,
	MCPI_X3 = 55,
	MCPI_FINAL = 65
};

/* global hits */
volatile double ghit;
/* are mutexes and global variables initialized */
static volatile int init = FALSE;
/* Used to implement critical region for ghit */
lock_t mutex;
/* Used to implement critical region for threads_done */
lock_t done_mutex;
/* Number of threads done with computation. */
static volatile int threads_done;

double my_rand(double x) {
	x = x * 1048576;
	x = 1.0 * (((1629 * (int)x) + 1) % 1048576);
	return x / 1048576.0;
}

void addsum(int hits) {
	lock_acquire(&mutex);
	ghit += hits;
	yield();
	lock_release(&mutex);
}

void print_res(int py, int px, double res) {
	print_str(py, px, "pi=       ");
	print_int(py, px + 3, (int)(res * RES_GRAN));
}

int is_hit(double x, double y) {
	double x2, y2, sum2;

	/*
	 * Test context switch mechanism.
	 */
	x2 = x * x;
	asm volatile("pushal ; call yield ; popal");
	y2 = y * y;

	sum2 = x2 + y2;

	if (sum2 <= 1.0)
		return TRUE;
	else
		return FALSE;
}

void mcpi(int cnt, int py, int px) {
	double x, y;
	int i, hit = 0;

	x = 324535.12;
	y = 32562.461;

	for (i = 0; i < cnt; i++) {
		x = my_rand(x);
		y = my_rand(y);
		if (is_hit(x, y)) {
			hit++;
		}

		/* Computation does not contribute to results, but is done to
		 * reduce accuracy of result if context switch mechnaism is not
		 * working correctly
		 */
		is_hit(2.0, 2.0); // Never a hit

		if (i % 10 == 0) {
			print_str(py, px, "i%10:     ");
			print_int(py, px + 6, i / 10);
		}
	}
	addsum(hit);
}

/* Called by threads 1--3. */
double run_mcpi_threadN(int py, int px) {
	int pjob;

	/* Wait until mutex is initialized */
	while (init == FALSE) {
		yield();
	}

	pjob = DARTS / WORKERS;
	mcpi(pjob, py, px);

	lock_acquire(&done_mutex);
	threads_done++;
	lock_release(&done_mutex);

	return (4.0 * ghit) / DARTS;
}

void mcpi_thread0(void) {
	double res;
	int pjob;

	print_str(MCPI_Y, 0, "Thread  4-7 (MCPi)    : ");
	print_str(MCPI_Y, MCPI_X0, "running");

	/* This thread initializes the mutex, the other thread waits for init to
	 * become true */
	if (!init) {
		lock_init(&mutex);
		lock_init(&done_mutex);
		ghit = 0.0;
		threads_done = 0;
		init = TRUE;
	}

	/* Calculate number of darts. */
	pjob = DARTS / WORKERS + DARTS % WORKERS;

	/* Do computation. */
	mcpi(pjob, MCPI_Y, MCPI_X0);
	res = (4.0 * ghit) / DARTS;
	print_res(MCPI_Y, MCPI_X0, res);

	lock_acquire(&done_mutex);
	threads_done++;
	lock_release(&done_mutex);

	/* Spin wait until all threads are done */
	while (threads_done != 4) {
		yield();
	}

	res = (4.0 * ghit) / DARTS;
	print_str(MCPI_Y, MCPI_FINAL, "final");
	print_res(MCPI_Y, MCPI_FINAL + 6, res);

	exit();
}

void mcpi_thread1(void) {
	double res;

	print_str(MCPI_Y, MCPI_X1, "running");

	res = run_mcpi_threadN(MCPI_Y, MCPI_X1);
	print_res(MCPI_Y, MCPI_X1, res);

	exit();
}

void mcpi_thread2(void) {
	double res;

	print_str(MCPI_Y, MCPI_X2, "running");

	res = run_mcpi_threadN(MCPI_Y, MCPI_X2);
	print_res(MCPI_Y, MCPI_X2, res);

	exit();
}

void mcpi_thread3(void) {
	double res;

	print_str(MCPI_Y, MCPI_X3, "running");

	res = run_mcpi_threadN(MCPI_Y, MCPI_X3);
	print_res(MCPI_Y, MCPI_X3, res);
	exit();
}
