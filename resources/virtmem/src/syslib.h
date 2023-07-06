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
int mbox_open(uint32_t key);
int mbox_close(uint32_t q);
int mbox_stat(uint32_t q, uint32_t *count, uint32_t *space);
int mbox_recv(uint32_t q, msg_t *m);
int mbox_send(uint32_t q, msg_t *m);
int getchar(uint32_t *c);
int readdir(uint8_t *buf);
void loadproc(uint32_t location, uint32_t size);

#endif /* !SYSLIB_H */
