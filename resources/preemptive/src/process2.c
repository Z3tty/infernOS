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

#define MATH_LINE 7

void _start(void) {
	int i, res;

	for (i = 0; i <= 100; i++) {
		res = rec(i);

		print_str(MATH_LINE, 0, "PID ");
		print_int(MATH_LINE, 4, getpid());
		print_str(MATH_LINE, 5, " : 1 + ... + ");
		print_int(MATH_LINE, 18, i);
		print_str(MATH_LINE, 21, " = ");
		print_int(MATH_LINE, 24, res);

		ms_delay(700);
		yield();
	}
	exit();
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
