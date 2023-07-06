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
static int invoke_syscall(uint32_t i, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
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

int mbox_open(uint32_t key) {
	return invoke_syscall(SYSCALL_MBOX_OPEN, key, IGNORE, IGNORE);
}

int mbox_close(uint32_t q) {
	return invoke_syscall(SYSCALL_MBOX_CLOSE, q, IGNORE, IGNORE);
}

int mbox_stat(uint32_t q, uint32_t *count, uint32_t *space) {
	return invoke_syscall(SYSCALL_MBOX_STAT, q, (uint32_t)count, (uint32_t)space);
}

int mbox_recv(uint32_t q, msg_t *m) {
	return invoke_syscall(SYSCALL_MBOX_RECV, q, (uint32_t)m, IGNORE);
}

int mbox_send(uint32_t q, msg_t *m) {
	return invoke_syscall(SYSCALL_MBOX_SEND, q, (uint32_t)m, IGNORE);
}

int getchar(uint32_t *c) {
	return invoke_syscall(SYSCALL_GETCHAR, (uint32_t)c, IGNORE, IGNORE);
}

int readdir(uint8_t *buf) {
	return invoke_syscall(SYSCALL_READDIR, (uint32_t)buf, IGNORE, IGNORE);
}

void loadproc(uint32_t location, uint32_t size) {
	invoke_syscall(SYSCALL_LOADPROC, location, size, IGNORE);
}
