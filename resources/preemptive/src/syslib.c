/*
 * Implementation of the system call library for the user
 * processes. Each function implements a trap to the kernel.
 */

#include "common.h"
#include "syslib.h"
#include "util.h"

/*
 * 1.   Place system call number (i) in eax and arg1 in ebx
 * 2.   Trigger interrupt 48 (system call).
 * 3.   Return value is in eax after returning from interrupt.
 */
static int invoke_syscall(int i, int arg1) {
	int ret;

	asm volatile("int $48" /* 48 = 0x30 */
	             : "=a"(ret)
	             : "%0"(i), "b"(arg1));
	return ret;
}

void yield(void) {
	invoke_syscall(SYSCALL_YIELD, IGNORE);
}

void exit(void) {
	invoke_syscall(SYSCALL_EXIT, IGNORE);
}

int getpid(void) {
	return invoke_syscall(SYSCALL_GETPID, IGNORE);
}

int getpriority(void) {
	return invoke_syscall(SYSCALL_GETPRIORITY, IGNORE);
}

void setpriority(int p) {
	invoke_syscall(SYSCALL_SETPRIORITY, p);
}

int cpuspeed(void) {
	return invoke_syscall(SYSCALL_CPUSPEED, IGNORE);
}
