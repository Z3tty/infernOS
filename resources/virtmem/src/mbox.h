#ifndef MBOX_H
#define MBOX_H

#include "thread.h"

enum
{
	MAX_MBOX = 5,       /* max number of mailboxes */
	BUFFER_SIZE = 1024, /* mbox buffer size */

	/* Used by the debug functions */
	LINE = 1,
	COL = 40
};

typedef struct {
	int used; /* Number of processes which has opened this mbox */
	lock_t l; /* The mailbox works like a monitor  */
	condition_t moreSpace, moreData;
	int count; /* Number of messages in mailbox */
	int head;  /* Points to the first free byte in the buffer */
	/* points to oldest message (first to be recived) in buffer */
	int tail;
	char buffer[BUFFER_SIZE];
} mbox_t;

/* Initialize mailbox system, called by kernel on startup  */
void mbox_init(void);

/*
 * Open a mailbox with the key 'key'.
 * Returns a mailbox handle which must be used to identify this
 * mailbox in the following functions (parameter q).
 */
int mbox_open(int key);

/* Closes the mailbox with handle q */
int mbox_close(int q);

/*
 * Get number of messages (count) and number of bytes available in the
 * mailbox buffer (space). Note that the buffer is also used for
 * storing the message headers, which means that a message will
 * take sizeof(mbox_t) + m->size bytes in the buffer.
 */
int mbox_stat(int q, uint32_t *count, uint32_t *space);

/* Fetch a mailbox from queue q and store it in m */
int mbox_recv(int q, msg_t *m);

/* Insert m into the mailbox q */
int mbox_send(int q, msg_t *m);

/* Debug function */
void print_mbox_status(void);

#endif /* !MBOX_H */
