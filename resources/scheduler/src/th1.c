/* Simple counter program, to check the functionality of yield().
 * Print time in seconds.
 */

#include "scheduler.h"
#include "th.h"
#include "util.h"

#define TIME_LINE 24
#define PROGRESS_LINE 19
#define MHZ 500 /* CPU clock rate */

static void print_counter(void);

/*
 * This thread runs indefinitely, which means that the
 * scheduler should never run out of processes.
 */
void clock_thread(void) {
	unsigned int time;
	unsigned long long int ticks;
	unsigned int start_time;

	/* To show time since last boot, remove all references to start_time */
	ticks = get_timer() >> 20;       /* divide on 2^20 = 10^6 (1048576) */
	start_time = ((int)ticks) / MHZ; /* divide on CPU clock frequency in
	                                  * megahertz */
	while (1) {
		ticks = get_timer() >> 20; /* divide on 2^20 = 10^6 (1048576) */
		time = ((int)ticks) / MHZ; /* divide on CPU clock frequency in
		                            * megahertz */
		print_str(TIME_LINE, 50, "Time (in seconds) : ");
		print_int(TIME_LINE, 70, time - start_time);
		print_counter();
		yield();
	}
}

static void print_counter(void) {
	static int counter = 0;

	print_str(PROGRESS_LINE, 0, "Thread  1 (time)      : ");
	print_int(PROGRESS_LINE, 25, counter++);
}
