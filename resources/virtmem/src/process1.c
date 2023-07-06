/*
 * Just "flies a plane" over the screen.
 *
 * Listens for messages in the mailbox with key 1, and fires a bullet
 * each time a message can be read.
 *
 */
#include "common.h"
#include "screen.h"
#include "syslib.h"
#include "util.h"

#define COMMAND_MBOX 1

static char picture[PLANE_ROWS][PLANE_COLUMNS + 1] = {
	"     ___       _  ",
	" | __\\_\\_o____/_| ",
	" <[___\\_\\_-----<  ",
	" |  o'            "};

static void draw_plane(int locX, int loc_y);
static void draw_bullet(int locX, int loc_y, int bullet);

int main(void) {
	int loc_x = PLANE_LOC_X_MAX;
	int loc_y = PLANE_LOC_Y_MAX;
	int fired = FALSE;
	int bullet_x = -1;
	int bullet_y = -1;
	msg_t m;               /* "fire bullet" message */
	uint32_t count, space; /* number of messages, free space in buffer */
	int c, q;

	/* open command mailbox, to communicate with the shell */
	q = mbox_open(COMMAND_MBOX);
	while (1) {
		loc_x -= 1;
		if (loc_x < PLANE_LOC_X_MIN) {
			loc_x = PLANE_LOC_X_MAX;
		}
		/* draw plane */
		draw_plane(loc_x, loc_y);

		/* mbox opened and bullet not fired? */
		if ((q >= 0) && (!fired)) {
			/*
			 * recive number of messages and free space in
			 * command mbox
			 */
			mbox_stat(q, &count, &space);
			if (count > 0) {
				/* if messages in mbox, recive first command */
				mbox_recv(q, &m);
				if (m.size != 0) {
					scrprintf(PLANE_ERROR_Y, PLANE_ERROR_X,
					          "Error: Invalid msg "
					          "size...exiting");
					exit();
				}

				/* fire bullet */
				fired = TRUE;
				if (loc_x < PLANE_BULLET_X_MIN)
					bullet_x = PLANE_LOC_X_MAX;
				else
					bullet_x = loc_x - 2;
				bullet_y = loc_y + 2;
			}
		}

		if (fired) {
			/* erase bullet */
			draw_bullet(bullet_x, bullet_y, FALSE);

			/* if bullet hit a character at screen[X-1, Y] */
			if ((bullet_x - 1 >= 0) && ((c = peek_screen(bullet_x - 1, bullet_y)) != 0) && (c != ' ')) {
				/*
				 * erase bullet and character at
				 * screen[X-1, Y]
				 */
				draw_bullet(bullet_x - 1, bullet_y, FALSE);
				fired = FALSE;
			}
			/* if bullet hit a character at screen[X-2, Y] */
			else if ((bullet_x - 2 >= 0) && ((c = peek_screen(bullet_x - 2, bullet_y)) != 0) && (c != ' ')) {
				/*
				 * erase bullet and character at
				 * screen[X-2, Y]
				 */
				draw_bullet(bullet_x - 2, bullet_y, FALSE);
				fired = FALSE;
			}
			/* bullet did not hit a character */
			else {
				bullet_x -= 2;
				if (bullet_x < 0)
					fired = FALSE;
				else
					draw_bullet(bullet_x, bullet_y, TRUE);
			}
		}
		ms_delay(250);
	} /* end forever loop */

	if (q >= 0) { /* should not be reached */
		mbox_close(q);
	}

	return 0;
}

static void draw_plane(int loc_x, int loc_y) {
	int i, j;
	for (i = 0; i < PLANE_COLUMNS; i++)
		for (j = 0; j < PLANE_ROWS; j++)
			if (loc_x + i >= PLANE_BULLET_X_MIN)
				scrprintf(loc_y + j, loc_x + i, "%c", picture[j][i]);
}

static void draw_bullet(int loc_x, int loc_y, int bullet) {
	if (bullet)
		scrprintf(loc_y, loc_x, "<=");
	else
		scrprintf(loc_y, loc_x, "  ");
}
