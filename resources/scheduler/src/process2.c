/*
 * Simple counter program, to check the functionality of yield().
 * Each process prints their invocation counter on a separate line
 */

#include "common.h"
#include "syslib.h"
#include "util.h"

static void print_counter(int done);
static int rec(int n);

void _start(void) {
	int i;

	for (i = 0; i <= 100; i++) {
		print_str(10, 0, "Did you know that 1 + ... + ");
		print_int(10, 28, i);
		print_str(10, 31, " = ");
		print_int(10, 34, rec(i));
		print_counter(FALSE);
		delay(DELAY_VAL);
		yield();
	}
	print_counter(TRUE);
	exit();
}

static void print_counter(int done) {
	static int counter = 0;

	print_str(24, 0, "Process 2 (Math)      : ");
	if (done) {
		print_str(24, 25, "Exited");
	}
	else {
		print_int(24, 25, counter++);
	}
}

/* calculate 1 + ... + n */
static int rec(int n) {
	if (n % 37 == 0) {
		yield();
	}
	if (n == 0) {
		return 0;
	}
	else {
		return n + rec(n - 1);
	}
}
