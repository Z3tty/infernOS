/*  kernel.c
 */
#include "common.h"
#include "kernel.h"
#include "scheduler.h"
#include "th.h"
#include "util.h"

// Statically allocate some storage for the pcb's
pcb_t pcb[NUM_TOTAL];
// Ready queue and pointer to currently running process
pcb_t *current_running;

/* This is the entry point for the kernel.
 * - Initialize pcb entries
 * - set up stacks
 * - start first thread/process
 */
void _start(void) {
	/* Declare entry_point as pointer to pointer to function returning void
	 * ENTRY_POINT is defined in kernel h as (void(**)()0xf00)
	 */
	void (**entry_point)() = ENTRY_POINT;

	// load address of kernel_entry into memory location 0xf00
	*entry_point = kernel_entry;

	clear_screen(0, 0, 80, 25);
	print_str(0,0, "Hello world, this is your kernel speaking.");
	while (1)
		;
}

/*  Helper function for kernel_entry, in entry.S. Does the actual work
 *  of executing the specified syscall.
 */
void kernel_entry_helper(int fn) {
}
