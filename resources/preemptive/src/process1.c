/* Just "flies a plane" over the screen. */
#include "common.h"
#include "screen.h"
#include "syslib.h"
#include "util.h"

static char picture[PLANE_ROWS][PLANE_COLUMNS + 1] = {
	"     ___       _  ",
	" | __\\_\\_o____/_| ",
	" <[___\\_\\_-----<  ",
	" |  o'            "};

static void draw(int loc_x, int loc_y, int plane);

void _start(void) {
	int count = 0, loc_x = 80, loc_y = 1;
	unsigned int pri, pid;

	while (1) {
		/* erase plane */
		draw(loc_x, loc_y, FALSE);
		loc_x -= 1;
		if (loc_x < -20) {
			loc_x = 80;
		}
		/* draw plane */
		draw(loc_x, loc_y, TRUE);
		if (count++ % 100) {
			/* print pid and priority */
			pid = getpid();
			pri = getpriority();
			print_str(PLANE_LINE, PLANE_PID_STR, "Process ");
			print_int(PLANE_LINE, PLANE_PID, pid);
			print_str(PLANE_LINE, PLANE_PRI_STR, "priority ");
			print_int(PLANE_LINE, PLANE_PRI, pri);
			if (pri < 64) {
				/* increae prioty (make plane go faster) */
				setpriority(pri + 1);
			}
			else {
				/* decreae priority (make plane go slower) */
				setpriority(10);
			}
		}
		ms_delay(250);
	}
}

/* draw plane */
static void draw(int loc_x, int loc_y, int plane) {
	int i, j;

	for (i = 0; i < PLANE_COLUMNS; i++) {
		for (j = 0; j < PLANE_ROWS; j++) {
			if (plane == TRUE) {
				print_char(loc_y + j, loc_x + i, picture[j][i]);
			}
			else {
				print_char(loc_y + j, loc_x + i, ' ');
			}
		}
	}
}
