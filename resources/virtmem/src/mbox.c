/*
 * Implementation of the mailbox.
 *
 * Implementation notes:
 *
 * The mailbox is protected with a lock to make sure that only
 * one process is within the queue at any time.
 *
 * It also uses condition variables to signal that more space or
 * more messages are available.
 *
 * In other words, this code can be seen as an example of implementing a
 * producer-consumer problem with a monitor and condition variables.
 *
 * Note that this implementation only allows keys from 0 to 4
 * (key >= 0 and key < MAX_Q).
 *
 * The buffer is a circular array.
 */

#include "common.h"
#include "mbox.h"
#include "thread.h"
#include "util.h"

mbox_t Q[MAX_MBOX];

static void buffer_to_msg(char *q, int start, char *s, int bytes);
static void msg_to_buffer(char *msg, int bytes, char *buf, int start);
static void print_trace(char *s, int q, int msgsize);

/*
 * Returns the number of bytes available in the queue Note: Mailboxes
 * with count=0 messages should have head=tail, which means that
 * we return BUFFER_SIZE bytes.
 */
static int space_available(mbox_t *q) {
	if ((q->tail == q->head) && (q->count != 0)) {
		/* Message in the queue, but no space  */
		return 0;
	}

	if (q->tail > q->head) {
		/* Head has wrapped around  */
		return q->tail - q->head;
	}
	/* Head has a higher index than tail  */
	return q->tail + BUFFER_SIZE - q->head;
}

/* Initialize mailbox system, called by kernel on startup  */
void mbox_init(void) {
	int i;

	for (i = 0; i < MAX_MBOX; i++) {
		Q[i].used = 0;
		lock_init(&Q[i].l);
		condition_init(&Q[i].moreSpace);
		condition_init(&Q[i].moreData);
		/*
		 * head, tail and count set by open if used == 0 Process X and
		 * process Y could both call mbox_close setting count = 0. But
		 * the buffer is not flushed.
		 */
	}
}

/*
 * Open a mailbox with the key 'key'.  Returns a mailbox handle which
 * must be used to identify this mailbox in the following
 * functions (parameter q).
 */
int mbox_open(int key) {
	if (key < MAX_MBOX && key >= 0) {
		lock_acquire(&Q[key].l);
		if (Q[key].used == 0) {
			/*
			 * The first time this mailbox is opened. Make
			 * sure that it's empty and cleaned up.
			 */
			Q[key].head = 0;
			Q[key].tail = 0;
			Q[key].count = 0;
		}
		Q[key].used++;
		lock_release(&Q[key].l);
		return key;
	}
	else {
		/* Illegal key */
		return -1;
	}
}

/* Close the mailbox with handle q  */
int mbox_close(int q) {
	lock_acquire(&Q[q].l);
	Q[q].used--;
	lock_release(&Q[q].l);
	return 1;
}

/*
 * Get number of messages (count) and number of bytes available in the
 * mailbox buffer (space). Note that the buffer is also used for
 * storing the message headers, which means that a message will
 * take MSG_T_HEADER + m->size bytes in the buffer.
 * (MSG_T_HEADER = sizeof(msg_t header))
 */
int mbox_stat(int q, uint32_t *count, uint32_t *space) {
	lock_acquire(&Q[q].l);
	*count = Q[q].count;
	*space = space_available(&Q[q]);
	lock_release(&Q[q].l);
	return 1;
}

/* Fetch a message from queue 'q' and store it in 'm'  */
int mbox_recv(int q, msg_t *m) {
	lock_acquire(&Q[q].l);
	print_trace("Recv", q, -1);

	/* If no messages available, wait until there is one  */
	while (Q[q].count == 0) {
		condition_wait(&Q[q].l, &Q[q].moreData);
	}

	/* copy header from mbox.buffer to m */
	buffer_to_msg(Q[q].buffer, Q[q].tail, (char *)&m->size, MSG_T_HEADER_SIZE);

	/* Move tail to the body of message */
	Q[q].tail = (Q[q].tail + MSG_T_HEADER_SIZE) % BUFFER_SIZE;

	/* Copy body of message from mbox.buffer to m->body */
	buffer_to_msg(Q[q].buffer, Q[q].tail, (char *)&m->body[0], m->size);

	/* Move tail to the next message */
	Q[q].tail = (Q[q].tail + MSG_SIZE(m) - MSG_T_HEADER_SIZE) % BUFFER_SIZE;

	/* Freeing space can satisy more than one writter */
	condition_broadcast(&Q[q].moreSpace);

	Q[q].count--;
	lock_release(&Q[q].l);

	return 1;
}

/* Insert 'm' into the mailbox 'q'  */
int mbox_send(int q, msg_t *m) {
	int msgSize = MSG_SIZE(m);

	lock_acquire(&Q[q].l);
	print_trace("Send", q, msgSize);

	/* Wait until there is enough space  */
	while (space_available(&Q[q]) < msgSize) {
		condition_wait(&Q[q].l, &Q[q].moreSpace);
	}

	/* copy from message m (header and body) to Q[q].buffer */
	msg_to_buffer((char *)m, msgSize, Q[q].buffer, Q[q].head);
	Q[q].head = (Q[q].head + msgSize) % BUFFER_SIZE;

	/* Send of one message can only satisfy one reader. */
	condition_signal(&Q[q].moreData);
	Q[q].count++;
	lock_release(&Q[q].l);
	return 1;
}

/* Debug function */
static void print_trace(char *s, int q, int msgSize) {
#ifdef DEBUG
	static int count = 0;
	count++;
	scrprintf(count % 10, 40, "%-5d%-5d%-10s%-10d%-10d", count / 10, q, s, space_available(&Q[q]), msgSize);
#endif /* DEBUG */
}

void print_mbox_status(void) {
	int i;

	scrprintf(LINE - 1, COL, "%-8s%-8s%-8s%-8s%-8s", "Mbox", "Used", "Head", "Tail", "Count");

	for (i = 0; i < MAX_MBOX; i++)
		scrprintf(LINE - 1, COL, "%-8d%-8d%-8d%-8d%-8d", i, Q[i].used, Q[i].head, Q[i].tail, Q[i].count);
}

/* Helper functions to copy messages to/from a circular buffer */

/*
 * copy bytes from buf, starting at index start to msg
 */
static void buffer_to_msg(char *buf, int start, char *msg, int bytes) {
	int i;

	for (i = 0; i < bytes; i++)
		msg[i] = buf[(start + i) % BUFFER_SIZE];
}

/*
 * copy bytes from msg, to buf, starting at start
 */
static void msg_to_buffer(char *msg, int bytes, char *buf, int start) {
	int i;

	for (i = 0; i < bytes; i++)
		buf[(start + i) % BUFFER_SIZE] = msg[i];
}
