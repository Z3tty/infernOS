/*
 * Simple counter program, to check the functionality of yield().
 * Print time in seconds.
 */

#include "scheduler.h"
#include "screen.h"
#include "th.h"
#include "util.h"

/*
 * This thread runs indefinitely, which means that the
 * scheduler should never run out of processes.
 */
void clock_thread(void) {
	unsigned int time;
	unsigned long long int ticks;
	unsigned int start_time;

	unsigned int mhz = cpuspeed();

	/* To show time since last boot, remove all references to start_time */
	ticks = get_timer() >> 20; /* divide on 2^20 = 10^6 (1048576) */
	/* divide by CPU clock frequency in megahertz */
	start_time = ((int)ticks) / mhz;

	while (1) {
		/* divide by 2^20 = 10^6 (1048576) */
		ticks = get_timer() >> 20;
		/* divide by CPU clock frequency in megahertz */
		time = ((int)ticks) / mhz;
		print_str(CLOCK_LINE, CLOCK_STR, "Time (in seconds) : ");
		print_int(CLOCK_LINE, CLOCK_VALUE, time - start_time);
		print_status();
		yield();
	}
}
