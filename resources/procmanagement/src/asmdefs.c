#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGNAME "asmdefs"

#define ADEF(stag, member)                                                     \
	do {                                                                   \
		char name[] = #stag "_" #member;                               \
		char *p = name;                                                \
		while (*p != '\0') {                                           \
			*p = toupper(*p);                                      \
			++p;                                                   \
		}                                                              \
		if (printf("#define %s %ld\n", name,                           \
			offsetof(struct stag, member))                      \
			< 0) {                                                     \
			fprintf(stderr,                                        \
				PROGNAME ": printing to stdout failed\n");     \
			exit(EXIT_FAILURE);                                    \
		}                                                              \
	} while (0)

#include "kernel.h"

int main(int argc, char *argv[]) {
	ADEF(pcb, pid);
	ADEF(pcb, is_thread);
	ADEF(pcb, user_stack);
	ADEF(pcb, kernel_stack);
	ADEF(pcb, base_kernel_stack);
	ADEF(pcb, disable_count);
	ADEF(pcb, preempt_count);
	ADEF(pcb, nested_count);
	ADEF(pcb, start_pc);
	ADEF(pcb, ds);
	ADEF(pcb, cs);
	ADEF(pcb, first_time);
	ADEF(pcb, priority);
	ADEF(pcb, status);
	ADEF(pcb, yield_count);
	ADEF(pcb, int_controller_mask);
	ADEF(pcb, base);
	ADEF(pcb, limit);
	ADEF(pcb, wakeup_time);
	ADEF(pcb, next_blocked);
	ADEF(pcb, page_directory);
	ADEF(pcb, next);
	ADEF(pcb, previous);
	return EXIT_SUCCESS;
}
