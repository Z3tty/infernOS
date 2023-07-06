#include "common.h"
#include "print.h"
#include "screen.h"
#include "syslib.h"
#include "util.h"

#define COMMAND_MBOX 1 /* mbox to send commands to process1 (plane) */

/* Print character in the shell window */
static int shprintf(char *in, ...);

/*
 * Read characters from command line into 'line'. This function calls
 * getchar() to recive a character. It returns when either 'max'
 * characters are read or "enter" is read
 */
static void read_line(int max, char *line);

/*
 * Parse command in 'line'. Arguments are stored in argv. Returns
 * number of arguments
 */
static int parse_line(char *line, char *argv[SHELL_SIZEX]);

/* cursor coordinate */
int cursor = 0;

int main(void) {
	int q, p, n;
	int rc;
	unsigned char buf[SECTOR_SIZE]; /* directory buffer */
	struct directory_t *dir = (struct directory_t *)buf;
	int argc;                   /* argument count */
	char *argv[SHELL_SIZEX];    /* argument vector */
	char line[SHELL_SIZEX + 1]; /* unparsed command */

	shprintf("D-240 Shell Version 0.00001\n\n");

	/* open mbox for communication with plane */
	q = mbox_open(COMMAND_MBOX);

	while (1) {
		shprintf("# ");
		/* read command into line[] */
		read_line(SHELL_SIZEX - 3, line);

		/*
		 * Parses command in line[] into *argv[], and returns
		 * number of arguments.
		 */
		argc = parse_line(line, argv);
		/* if no arguments goto beginning of loop */
		if (argc == 0) {
			continue;
		}

		/* clear screen */
		if (same_string("clear", argv[0])) {
			if (argc == 1) {
				/* Clear shell window */
				clear_screen(SHELL_MINX, SHELL_MINY, MAXX, MAXY);
				cursor = 0;
			}
			else {
				shprintf("Usage: %s\n", argv[0]);
			}
		}
		/*
		 * Send "fire" command through the mailbox to any
		 * listening plane
		 */
		else if (same_string("fire", argv[0])) {
			if (argc == 1) {
				if (q >= 0) {
					msg_t m;
					m.size = 0;
					mbox_send(q, &m);
				}
				else {
					shprintf("Mailboxes not available\n");
				}
			}
			else
				shprintf("Usage: %s\n", argv[0]);
		}
		/* Close shell */
		else if (same_string("exit", argv[0])) {
			if (argc == 1) {
				shprintf("Goodbye");
				exit();
			}
			else
				shprintf("Usage: %s\n", argv[0]);
		}
		else if (same_string("ls", argv[0])) {
			if (argc == 1) {
				rc = readdir(buf);
				if (rc < 0) {
					shprintf("Read error.\n");
					continue;
				}
				dir = (struct directory_t *)buf;

				/* number of proceses in directory */
				p = 0;
				/* parse directory */
				while (dir->location != 0) {
					shprintf("process %d - location: %d, size: %d\n", p++, dir->location, dir->size);
					dir++;
				}

				if (p == 0) {
					shprintf("Process not found.\n");
				}
				else {
					shprintf("%d process(es).\n", p);
				}
			}
			else {
				shprintf("Usage: %s\n", argv[0]);
			}
		}
		else if (same_string("load", argv[0])) {
			if (argc == 2) {
				rc = readdir(buf);
				if (rc < 0) {
					shprintf("Read error.\n");
					continue;
				}

				dir = (struct directory_t *)buf;
				n = atoi(argv[1]);
				for (p = 0; p < n && dir->location != 0; p++, dir++)
					;

				if (dir->location != 0) {
					loadproc(dir->location, dir->size);
					shprintf("Done.\n");
				}
				else {
					shprintf("File not found.\n");
				}
			}
			else {
				shprintf("usage: %s  'process number'\n", argv[0]);
			}
		}
		else if (same_string("ps", argv[0])) {
			shprintf("%s : Command not implemented.\n", argv[0]);
		}
		else if (same_string("kill", argv[0])) {
			shprintf("%s : Command not implemented.\n", argv[0]);
		}
		else if (same_string("suspend", argv[0])) {
			shprintf("%s : Command not implemented.\n", argv[0]);
		}
		else if (same_string("resume", argv[0])) {
			shprintf("%s : Command not implemented.\n", argv[0]);
		}
		else {
			shprintf("%s : Command not found.\n", argv[0]);
		}
	}

	return -1;
}

/*
 * Parse command in 'line'.
 * Pointers to arguments (in line) are stored in argv.
 * Returns number of arguments.
 */
static int parse_line(char *line, char *argv[SHELL_SIZEX]) {
	int argc;
	char *s = line;

	argc = 0;
	while (1) {
		/* Jump over blanks */
		while (*s == ' ') {
			/* All blanks are replaced with '\0' */
			*s = '\0';
			s++;
		}
		/* End of line reached, no more arguments */
		if (*s == '\0')
			return argc;
		else {
			/* Store pointer to argument in argv */
			argv[argc++] = s;
			/* And goto next blank character (or end of line) */
			while ((*s != ' ') && (*s != '\0'))
				s++;
		}
	}
}

/*
 * Read characters from command line into 'line'. This function calls
 * getchar() to recive a character. It returns when either 'max' characters
 * are read or "enter" is read
 */
static void read_line(int max, char *line) {
	int i = 0;
	uint32_t c;

	ASSERT2(max <= SHELL_SIZEX, "Request too long");
	do {
		getchar(&c);

		switch (c) {
		case '\r':
			line[i] = '\0';
			shprintf("\n");
			return;
		case '\b':
			if (i > 0) {
				line[--i] = '\0';
				shprintf("\b");
			}
			break;
		default:
			line[i++] = c;
			shprintf("%c", c);
		}
	} while (i < max);
	line[i] = '\0';
	shprintf("\n");
}

/* Shell write */
static int shwrite(void *drop, char c) {
	int x;
	int y;

	x = cursor % SHELL_SIZEX;
	y = cursor / SHELL_SIZEX;
	invert_color(y + SHELL_MINY, x + SHELL_MINX);

	switch (c) {
	case '\n':
		cursor += SHELL_SIZEX - cursor % SHELL_SIZEX;
		break;
	case '\b':
		cursor -= 1;
		x = cursor % SHELL_SIZEX;
		y = cursor / SHELL_SIZEX;
		scrprintf(y + SHELL_MINY, x + SHELL_MINX, " ");
		break;
	default:
		scrprintf(y + SHELL_MINY, x + SHELL_MINX, "%c", c);
		cursor++;
	}

	/* if cursor outside window then scroll window */
	if (cursor >= SHELL_SIZEX * SHELL_SIZEY) {
		scroll(SHELL_MINX, SHELL_MINY, MAXX, MAXY);
		cursor -= SHELL_SIZEX;
	}

	x = cursor % SHELL_SIZEX;
	y = cursor / SHELL_SIZEX;
	invert_color(y + SHELL_MINY, x + SHELL_MINX);

	return 1;
}

static int shprintf(char *in, ...) {
	va_list args;

	struct output out = {.write = shwrite, /* write function */
	                     .data = NULL};

	va_start(args, in);

	return uprintf(&out, in, args);
}
