#include "kernel.h"
#include "syslib.h"
// Call kernel_entry with arg i
// - push arg
// - call addr (memory[0x0f00])
#define SYSCALL(i) ((*entry_point)(i))

// Declare entry
static void (**entry_point)() = ENTRY_POINT;

/*
 * call kernel_entry (in kernel.c) with argument SYSCALL_YIELD
 *
 * yield() in assembly code:
 * movl entry_point,%eax   # move address of entry_point to %eax
 * pushl $0        # push argument
 * movl (%eax),%eax    # move memory[entry_point] into %eax
 * call *%eax          # call address pointed to by %eax
 * addl $4,%esp            # pop argument
 * ret                     # return to caller
 */
void yield(void) {
	SYSCALL(SYSCALL_YIELD);
}

/* call kernel_entry with arguemnt SYSCALL_EXIT */
void exit(void) {
	SYSCALL(SYSCALL_EXIT);
}
