/*
 * process2.c
 *
 * Simple process which does some calculations and exits.
 */

#include "common.h"
#include "screen.h"
#include "syslib.h"
#include "util.h"

static int rec(int n);

int main(void) {
	int i, res;

	for (i = 0; i <= 100; i++) {
		res = rec(i);

		scrprintf(MATH_LINE, 0, "PID %d : 1 + ... + %d = %d ", getpid(), i, res);

		ms_delay(700);
		yield();
	}
	exit();

	return 0;
}

/* calculate 1+ ... + n */
static int rec(int n) {
	if (n % 37 == 0)
		yield();

	if (n == 0)
		return 0;
	else
		return n + rec(n - 1);
}
