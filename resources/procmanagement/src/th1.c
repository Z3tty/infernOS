/*
 * loader_thread is used to load the shell. clock_thread is a thread
 * which runs indefinitely.
 */
#include "kernel.h"
#include "mbox.h"
#include "scheduler.h"
#include "sleep.h"
#include "th.h"
#include "usb/scsi.h"
#include "usb/usb_hub.h"
#include "util.h"

#define MHZ 2000 /* CPU clock rate */

/*
 * This thread is started to load the user shell, which is the first
 * process in the directory.
 */
void loader_thread(void) {
	char buf[SECTOR_SIZE]; /* buffer to hold directory */
	struct directory_t *dir = (struct directory_t *)buf;

	/* read process directory sector into buf */
	while (!scsi_up())
		yield();

	readdir(buf);
	/* only load the first process, we assume it's the shell */
	if (dir->location != 0) {
		loadproc(dir->location, dir->size);
	}
	exit();
}

/*
 * This thread runs indefinitely, which means that the scheduler
 * should never run out of processes.
 */
void clock_thread(void) {
	unsigned int time;
	unsigned long long ticks, start_ticks;

	start_ticks = get_timer() >> 20;
	while (1) {
		ticks = get_timer() >> 20; /* multiply with 2^10 */
		time = ((int)(ticks - start_ticks)) / MHZ;
		print_status(time);
		yield();
	}
}

/*
 * This thread periodically scans USB hub ports for new connected
 * devices.
 */
void usb_thread(void) {
	while (1) {
		msleep(100);
		usb_hub_scan_ports();
	}
}
