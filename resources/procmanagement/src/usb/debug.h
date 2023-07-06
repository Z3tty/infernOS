#ifndef DEBUG_H
#define DEBUG_H

#include "../util.h"

#if 0
#define DEBUG_NAME(x) static const char debug_name[] = x;

#define DEBUG_STATUS(x) (((x) >= 0) ? "OK" : "ERR")

#define DEBUG(args...)                                                         \
	{                                                                      \
		rsprintf("%8s ", debug_name);                                  \
		rsprintf(args);                                                \
		rsprintf("\n");                                                \
	}
#else
#define DEBUG_NAME(x)
#define DEBUG(...)
#endif

#endif
