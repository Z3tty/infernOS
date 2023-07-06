/*
 * Example process sending and receiving messages.
 */

#include "common.h"
#include "screen.h"
#include "syslib.h"
#include "util.h"

/*
 * Note: in process3.c FLOW_OUT = process4.c FLOW_IN
 */
#define QUEUE 2    /* shared buffer (p3 send message m to reciver p4) */
#define FLOW_OUT 3 /* mailbox p4 recives token from */
#define FLOW_IN 4  /* mailbox p4 sends token to */

#define MAX_MSG_SIZE 256
#define ACTUAL_MSG_SIZE(n) ((n) + sizeof(int))
/* allocate memory for one message with a body of 256 bytes */
char space[ACTUAL_MSG_SIZE(MAX_MSG_SIZE)];
msg_t *m = (msg_t *)space;
/* token is a message with a body of zero bytes (contains only a header) */
msg_t token;

int main(void) {
	int q, fi, fo; /* mbox numbers */
	int size, c, i, j;
	/* should the process wait for token or send token */
	int wait = FALSE;
	uint32_t count, space;

	/* q: shared buffer, process 3 sends message m to process 4 */
	if ((q = mbox_open(QUEUE)) < 0)
		exit();
	/* process 3 recives token from fi (process 4 sends token to fi) */
	if ((fi = mbox_open(FLOW_IN)) < 0)
		exit();
	/* process3 send token to fo (process 4 recives token from fo) */
	if ((fo = mbox_open(FLOW_OUT)) < 0)
		exit();

	for (i = 0; i < 10000; i++) {
		/* Try to send and receive messages of sizes
		 * 128-255 bytes.
		 */

		/* print number of iterations */
		scrprintf(PROC4_LINE, 15, "Process 4");
		scrprintf(PROC4_LINE + 1, 15, "%d", i);
		size = 128 + i % 128;
		c = 'a' + i % 26;

		if (i < 5000) {
			if (wait) {
				/*
				 * wait until process3 sends token
				 * (when enough room in q)
				 */
				mbox_recv(fi, &token);
				if (token.size != 0) {
					scrprintf(PROC4_LINE, 15, "Error:   ");
					scrprintf(PROC4_LINE + 1, 15, "Invalid control msg");
					exit();
				}

				wait = FALSE;
			}
			else {
				mbox_stat(q, &count, &space);
				/*
				 * if no messages in shared buffer, send token
				 */
				if (count == 0) {
					token.size = 0;
					mbox_send(fo, &token);
					wait = TRUE;
				}
			}
		}
		else if (i == 5000) {
			mbox_send(fo, &token);
		}

		/* recive message m (block if no more messages) */
		mbox_recv(q, m);

		/* check if recived message is correct */
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

	scrprintf(PROC4_LINE, 15, "Process 4");
	scrprintf(PROC4_LINE + 1, 15, "Done   ");
	mbox_close(q);
	mbox_close(fi);
	mbox_close(fo);
	exit();

	return 0;
}
