#ifndef KERNEL_H
#define KERNEL_H

#include "common.h"

// Cast 0xf00 into a ptr->ptr->func returning void
// used in syslib to declare the entry point
#define ENTRY_POINT ((void (**)())0xf00)

// Consts
enum {
    // Number of threads and proc's initially started by kernel
    // MUST be changed when changing start_addr arr
    NUM_THREADS = 7,
    NUM_PROCS = 2,
    NUM_TOTAL = (NUM_THREADS + NUM_PROCS),
    // Syscalls
    SYSCALL_YIELD = 0,
    SYSCALL_EXIT,
    SYSCALL_COUNT,
    // Stack Constants
    STACK_MIN = 0x10000,
    STACK_MAX = 0x20000,
    STACK_OFFSET = 0x0ffc,
    STACK_SIZE = 0x1000
};

// Process control block (pcb) is used for storing info about threads/procs
typedef struct pcb {
    struct pcb *next, *previous; // next is used for ready queue
} pcb_t;

// Currently running process, also points to rq
extern pcb_t *current_running;

void kernel_entry(int fn);
void kernel_entry_helper(int fn);

#endif
