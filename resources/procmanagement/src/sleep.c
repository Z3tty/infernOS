/*
 * Contains functions to allow processes to sleep for some number of
 * milliseconds.
 */

#include "interrupt.h"
#include "scheduler.h"
#include "sleep.h"
#include "time.h"
#include "util.h"

void msleep(uint32_t msecs) {
	uint64_t time;

	time = get_timer();
	current_running->wakeup_time = time + (msecs * cpu_mhz * 1000);
	current_running->status = SLEEPING;
	yield();
	/*
	 * When we return here, we will have waited atleast <msecs>
	 * milliseconds.
	 */
}
