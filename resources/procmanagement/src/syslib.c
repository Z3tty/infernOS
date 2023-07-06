/*
 * Implementation of the system call library for the user
 * processes. Each function implements a trap to the kernel.
 */
#include "common.h"
#include "syslib.h"
#include "util.h"

/*
 * 1. Place system call number (i) in eax, arg1 in ebx, arg2 in ecx and
 *    arg 3 in edx.
 * 2. Trigger interrupt 48 (system call).
 * 3. Return value is in eax after returning from interrupt.
 */
static int invoke_syscall(int i, int arg1, int arg2, int arg3) {
	int ret;

	asm volatile("int $48" /* 48 = 0x30 */
	             : "=a"(ret)
	             : "%0"(i), "b"(arg1), "c"(arg2), "d"(arg3));
	return ret;
}

void yield(void) {
	invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE);
}

void exit(void) {
	invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE);
}

int getpid(void) {
	return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE);
}

int getpriority(void) {
	return invoke_syscall(SYSCALL_GETPRIORITY, IGNORE, IGNORE, IGNORE);
}

void setpriority(int p) {
	invoke_syscall(SYSCALL_SETPRIORITY, p, IGNORE, IGNORE);
}

int cpuspeed(void) {
	return invoke_syscall(SYSCALL_CPUSPEED, IGNORE, IGNORE, IGNORE);
}

int mbox_open(int key) {
	return invoke_syscall(SYSCALL_MBOX_OPEN, key, IGNORE, IGNORE);
}

int mbox_close(int q) {
	return invoke_syscall(SYSCALL_MBOX_CLOSE, q, IGNORE, IGNORE);
}

int mbox_stat(int q, uint32_t *count, uint32_t *space) {
	return invoke_syscall(SYSCALL_MBOX_STAT, q, (int)count, (int)space);
}

int mbox_recv(int q, msg_t *m) {
	return invoke_syscall(SYSCALL_MBOX_RECV, q, (int)m, IGNORE);
}

int mbox_send(int q, msg_t *m) {
	return invoke_syscall(SYSCALL_MBOX_SEND, q, (int)m, IGNORE);
}

int getchar(int *c) {
	return invoke_syscall(SYSCALL_GETCHAR, (int)c, IGNORE, IGNORE);
}

int readdir(char *buf) {
	return invoke_syscall(SYSCALL_READDIR, (int)buf, IGNORE, IGNORE);
}

int loadproc(int location, int size) {
	return invoke_syscall(SYSCALL_LOADPROC, location, size, IGNORE);
}
