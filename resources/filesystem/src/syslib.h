#ifndef SYSLIB_H
#define SYSLIB_H

#include "common.h"

enum
{
	IGNORE = 0
};

/* Prototypes for exported system calls */
void yield(void);
void exit(void);
int getpid(void);
int getpriority(void);
void setpriority(int);
int cpuspeed(void);
int mbox_open(int key);
int mbox_close(int q);
int mbox_stat(int q, uint32_t *count, uint32_t *space);
int mbox_recv(int q, msg_t *m);
int mbox_send(int q, msg_t *m);
int getchar(int *c);
int readdir(unsigned char *buf);
void loadproc(int location, int size);
void fs_mkfs(void);
int fs_open(const char *filename, int mode);
int fs_close(int fd);
int fs_read(int fd, char *buffer, int size);
int fs_write(int fd, char *buffer, int size);
int fs_lseek(int fd, int offset, int whence);
int fs_link(char *linkname, char *filename);
int fs_unlink(char *linkname);
int fs_stat(int fd, char *buffer);

#endif /* !SYSLIB_H */
