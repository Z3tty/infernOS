#ifndef SYSLIB_H
#define SYSLIB_H

#include "common.h"

enum
{
	IGNORE = 0
};

void yield(void);
void exit(void);
int getpid(void);
int getpriority(void);
void setpriority(int);
int cpuspeed(void);

#endif /* !SYSLIB_H */
