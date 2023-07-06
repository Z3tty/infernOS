#include "kernel.h"
#include "common.h"
#include "scheduler.h"
#include "util.h"

// Statically allocate space for pcb's
pcb_t pcb[NUM_TOTAL];
// RQ and pointer to current
pcb_t *current_running;

/*
 * Kernel entry point
 *  - Init PCBs
 *  - Setup stacks
 *  - Start first thread/proc
 * */
void _start(void) {
    // Declare entry point using ptr->ptr->func defined in kernel.h (0x0f00)
    void (**entry_point)() = ENTRY_POINT;
    // Load kernel_entry addr into mem at x0f00
    *entry_point = kernel_entry;
    clear_screen(0, 0, 80, 25);
    print_str(0, 0, "InfernOS kernel loaded");
    while(1) ;
}

void kernel_entry_helper(int fn) {
    // TODO
}
