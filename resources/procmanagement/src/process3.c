/*
 * process3.c and process4.c: Examples of message passing with flow control
 *
 * These two programs are a pair that communicate with each other.
 * This is process3, the sender.
 *
 * The core task of these programs is for process3 to send 1000 messages to
 * process4 over the q/QUEUE mailbox. These messages are generated
 * deterministically in sequence, and process4 verifies that it receives them as
 * exected. This tests your mailbox implementation's integrity.
 *
 * However, instead of simply sending and receiving messages concurrently,
 * process3 and process4 coordinate to take turns sending and receiving,
 * so that they alternate between filling the buffer and emptying it.
 * This flow-control is accomplished by using two more mailboxes:
 * fi/FLOW_IN and fo/FLOW_OUT. Each process also has a 'wait' state:
 *
 * - When either process is in 'wait' state, it reads from its FLOW_IN.
 *   This blocks until it receives a 'token' as a signal from the other process.
 *
 * - When not in the wait state, process3 sends messages in the QUEUE until
 *   there is no more space in the mailbox. Then it sends a token on FLOW_OUT,
 *   which is connected to process4's FLOW_IN. This token essentially means
 *   "I have filled the buffer. Now it's your turn."
 *
 * - When not in the wait state, process4 receives messages from the QUEUE
 *   until the mailbox is empty. Then it sends a token to process3.
 *   "I have emptied the buffer. Now it's your turn."
 *
 * This tests how your mailbox handles empty and full states. When empty, does
 * it block until a new message comes in? When full, does it block until there
 * is space?
 *
 * If everything is implemented correctly, these processes should send their
 * messages, print "Done" and exit cleanly.
 */

#include "common.h"
#include "screen.h"
#include "syslib.h"
#include "util.h"

/*
 * Mailboxes
 *
 * QUEUE is for process3 to send messages to process4.
 * FLOW_IN and FLOW_OUT are for sending tokens.
 * They are cross-connected:
 *
 * - process3 sends on its FLOW_OUT, which is process4's FLOW_IN.
 * - process4 sends on its FLOW_OUT, which is process3's FLOW_IN.
 */
#define QUEUE 2
#define FLOW_IN 3
#define FLOW_OUT 4

#define MAX_MSG_SIZE 256
#define ACTUAL_MSG_SIZE(n) ((n) + sizeof(int))

/* allocate memory for one message with a body of 256 bytes */
char space[ACTUAL_MSG_SIZE(MAX_MSG_SIZE)];
msg_t *m = (msg_t *)space;
msg_t token; /* a message with a body of one byte (only header) */

int main(void)
{
	int q, fi, fo; /* mbox numbers */
	int size, c, i, j;
	/* should the process wait for messages or send msgs */
	int wait = TRUE;
	uint32_t count,
		space; /* number of messages, available space in QUEUE */

	/* initialize mailboxes or exit on error */
	if ((q = mbox_open(QUEUE)) < 0)
		exit();
	if ((fi = mbox_open(FLOW_IN)) < 0)
		exit();
	if ((fo = mbox_open(FLOW_OUT)) < 0)
		exit();

	for (i = 0; i < 1000; i++) {
		/*
		 * Try to send and receive messages of sizes 128-255 bytes.
		 */

		/* print number of iterations */
		scrprintf(PROC3_LINE, 0, "Process 3");
		scrprintf(PROC3_LINE + 1, 0, "%d", i);

		/*
		 * Create message to send
		 *
		 * Size and content are set deterministically from 'i', so that
		 * the sequence of messages is predictable.
		 */
		size = 128 + i % 128;
		m->size = size;

		c = 'a' + i % 26;
		for (j = 0; j < size; j++)
			m->body[j] = c;

		/*
		 * Wait and flow-control logic
		 *
		 * If not waiting, process3 sends until the buffer is full.
		 * If not waiting, process4 reads until the buffer is empty.
		 * The tokens over fi/fo represent a "your turn" signal.
		 */
		if (wait) {
			/* Wait for signal from other process */
			mbox_recv(fi, &token);
			if (token.size != 0) {
				scrprintf(PROC3_LINE, 0, "Error:   ");
				scrprintf(PROC3_LINE + 1, 0,
					  "Invalid control msg");
				exit();
			}
			/* Leave wait state */
			wait = FALSE;

		} else {
			/* process3 non-wait state: send until buffer is full */
			mbox_stat(q, &count, &space);
			if (space < ACTUAL_MSG_SIZE(m->size)) {
				/* "I have filled the buffer. Your turn." */
				token.size = 0;
				mbox_send(fo, &token);
				wait = TRUE;
			}
		}

		/*
		 * Send message
		 *
		 * Note: we may have just finished filling the queue. We send on
		 * this iteration anyway. The send call should block until there
		 * is space.
		 */
		mbox_send(q, m);
	}

	/*
	 * Send one last "your turn" signal to process4
	 *
	 * We are done sending, but it is likely that process4 is still in its
	 * wait state, waiting on a signal from us to continue reading from the
	 * queue. Send that signal now.
	 */
	mbox_send(fo, &token);

	/* Finish up */
	scrprintf(PROC3_LINE, 0, "Process 3");
	scrprintf(PROC3_LINE + 1, 0, "Done   ");
	mbox_close(q);
	mbox_close(fi);
	mbox_close(fo);
	exit();

	return -1;
}
