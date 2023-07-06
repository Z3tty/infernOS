/*
 * process3.c and process4.c: Examples of message passing with flow control
 *
 * This is process4, the receiver. See process3 for a detailed description
 * of how these programs work togther.
 */

#include "common.h"
#include "syslib.h"
#include "util.h"
#include "screen.h"

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
#define FLOW_OUT 3
#define FLOW_IN 4

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
	/* should the process wait for token or send token */
	int wait = FALSE;
	uint32_t count, space;

	/* initialize mailboxes or exit on error */
	if ((q = mbox_open(QUEUE)) < 0)
		exit();
	if ((fi = mbox_open(FLOW_IN)) < 0)
		exit();
	if ((fo = mbox_open(FLOW_OUT)) < 0)
		exit();

	for (i = 0; i < 1000; i++) {
		/* Try to send and receive messages of sizes
		 * 128-255 bytes.
		 */

		/* print number of iterations */
		scrprintf(PROC4_LINE, 15, "Process 4");
		scrprintf(PROC4_LINE + 1, 15, "%d", i);
		size = 128 + i % 128;
		c = 'a' + i % 26;

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
				scrprintf(PROC4_LINE, 15, "Error:   ");
				scrprintf(PROC4_LINE + 1, 15,
					  "Invalid control msg");
				exit();
			}
			/* Leave wait state */
			wait = FALSE;

		} else {
			/* process4 non-wait state: receive until buffer is
			 * empty */
			mbox_stat(q, &count, &space);
			if (count == 0) {
				/* "I have emptied the buffer. Your turn." */
				token.size = 0;
				mbox_send(fo, &token);
				wait = TRUE;
			}
		}

		/*
		 * Receive message
		 *
		 * Note: we may have just finished emptying the queue. We
		 * receive on this iteration anyway. The receive call should
		 * block until there is a message.
		 */
		mbox_recv(q, m);

		/*
		 * Verify message
		 *
		 * Since the message size and content are deterministic based
		 * on 'i', we can verify that the mailbox has transferred them
		 * as expected.
		 */
		if (m->size != size) {
			scrprintf(PROC4_LINE, 15, "Error:   ");
			scrprintf(PROC4_LINE + 1, 15, "Invalid size");
			exit();
		}

		for (j = 0; j < size; j++)
			if (m->body[j] != c) {
				scrprintf(PROC4_LINE, 15, "Error:   ");
				scrprintf(PROC4_LINE + 1, 15, "Invalid data");
				exit();
			}
	}

	/* Finish up */
	scrprintf(PROC4_LINE, 15, "Process 4");
	scrprintf(PROC4_LINE + 1, 15, "Done   ");
	mbox_close(q);
	mbox_close(fi);
	mbox_close(fo);
	exit();

	return -1;
}
