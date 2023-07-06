#ifndef PRINT_H
#define PRINT_H

#include <stdarg.h>

/* Print output definition */
struct output {
	void *data;
	int (*write)(void *data, char c);
};

int uprintf(struct output *out, char *in, va_list args);

#endif
