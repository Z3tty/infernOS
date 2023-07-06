#include "common.h"
#include "fs.h"
#include "print.h"
#include "screen.h"
#include "syslib.h"
#include "util.h"

#define COMMAND_MBOX 1 /* mbox to send commands to process1 (plane) */

#define MAXX (SHELL_MINX + SHELL_SIZEX)
#define MAXY (SHELL_MINY + SHELL_SIZEY)

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

/* Add to cwd the path in 'path'. */
static void change_cwd(char *cwd, char *path);

/* Shell commands */
static void ls(char *path);
static void cat(char *filename);
static void more(char *filename);
static void stat(char *filename);

/* cursor coordinate */
int cursor = 0;

int main(void) {
	int ev;
	int q, p, n;
	int rc;
	unsigned char buf[SECTOR_SIZE]; /* directory buffer */
	struct directory_t *dir = (struct directory_t *)buf;
	int argc;                       /* argument count */
	char *argv[SHELL_SIZEX];        /* argument vector */
	char line[SHELL_SIZEX * 3 + 1]; /* unparsed command */
	char cwd[SHELL_SIZEX + 1];      /* current working directory */

	shprintf("D-240 Shell Version 0.00001\n\n");

	/* cwd is the root directory */
	cwd[0] = '/';
	cwd[1] = '\0';

	/* open mbox for communication with plane */
	q = mbox_open(COMMAND_MBOX);

	while (1) {
		shprintf("$ ");
		/* read command into line[] */
		read_line(SHELL_SIZEX * 3, line);

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
		else if (same_string("lse", argv[0])) {
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
		else if (same_string("mkdir", argv[0])) {
			if (argc == 2) {
				if ((ev = fs_mkdir(argv[1])) < 0)
					shprintf(" : error occured.\n");
			}
			else {
				shprintf("usage: %s  'directory name'\n", argv[0]);
				continue;
			}
		}
		else if (same_string("cd", argv[0])) {
			if (argc == 2) {
				if ((ev = fs_chdir(argv[1])) < 0)
					shprintf(" : error occured.\n");
				else
					change_cwd(cwd, argv[1]);
			}
			else {
				shprintf("usage: %s  'directory name'\n", argv[0]);
				continue;
			}
		}
		else if (same_string("rmdir", argv[0])) {
			if (argc == 2) {
				if ((ev = fs_rmdir(argv[1])) < 0)
					shprintf(" : error occured.\n");
			}
			else {
				shprintf("usage: %s  'directory name'\n", argv[0]);
				continue;
			}
		}
		else if (same_string("ls", argv[0])) {
			if (argc == 1) {
				ls(cwd);
			}
			else {
				shprintf("usage: %s\n", argv[0]);
				continue;
			}
		}
		else if (same_string("pwd", argv[0])) {
			if (argc == 1) {
				shprintf("%s\n", cwd);
			}
			else {
				shprintf("usage: %s\n", argv[0]);
				continue;
			}
		}
		else if (same_string("cat", argv[0])) {
			if (argc == 2) {
				cat(argv[1]);
			}
			else {
				shprintf("usage: %s 'file name'\n", argv[0]);
				continue;
			}
		}
		else if (same_string("more", argv[0])) {
			if (argc == 2) {
				more(argv[1]);
			}
			else {
				shprintf("usage: %s 'file name'\n", argv[0]);
				continue;
			}
		}
		else if (same_string("ln", argv[0])) {
			if (argc == 3) {
				if ((ev = fs_link(argv[1], argv[2])) < 0)
					shprintf(" : error occured.\n");
			}
			else {
				shprintf("usage: %s 'link name' 'file name'\n", argv[0]);
				continue;
			}
		}
		else if (same_string("rm", argv[0])) {
			if (argc == 2) {
				if ((ev = fs_unlink(argv[1])) < 0) {
					shprintf(" : error occured.\n");
				}
			}
			else {
				shprintf("usage: %s 'file name'\n", argv[0]);
				continue;
			}
		}
		else if (same_string("stat", argv[0])) {
			if (argc == 2) {
				stat(argv[1]);
			}
			else {
				shprintf("usage: %s 'file name'\n", argv[0]);
				continue;
			}
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
	int i = 0, c;

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

/* Extract every directory name out of a path. This consist of replacing every /
 * with '\0' */
static int parse_path(char *path, char *argv[SHELL_SIZEX]) {
	char *s = path;
	int argc = 0;

	argv[argc++] = path;

	while (*s != '\0') {
		if (*s == '/') {
			*s = '\0';
			argv[argc++] = (s + 1);
		}

		s++;
	}

	return argc;
}

/*
 * Add to cwd the path in 'path'
 */
static void change_cwd(char *cwd, char *path) {
	char *argv[SHELL_SIZEX];
	int argc, i;
	char *s, *p;

	if (*path == '/') { /* is absoulte */
		bcopy(path, cwd, strlen(path));
		return;
	}

	if (path[strlen(path) - 1] == '/') { /* Remove '/' at end of name */
		path[strlen(path) - 1] = '\0';
	}

	argc = parse_path(path, argv);
	if (same_string(cwd, "/"))
		s = cwd;
	else
		s = &cwd[strlen(cwd)]; /* end of line */

	for (i = 0; i < argc; i++) {
		if (same_string(argv[i], ".")) {
			; /* do nothing */
		}
		else if (same_string(argv[i], "..")) {
			/* Remove last part of the filename */
			if (*s == '/' && s != cwd)
				s--;

			/* Move backwards until / is reached */
			while (*s != '/') {
				s--;
			}
		}
		else {
			*s = '/';
			s++;

			p = argv[i];
			while (*p != '\0') {
				*s = *p;
				s++;
				p++;
			}
		}
	}

	if (s == cwd) {
		s++;
	}
	*s = '\0';
}

/*
 * ls - print out a unsorted list of filenames.
 * TODO: sort files.
 */
static void ls(char *cwd) {
	int fd, ev;
	struct dirent de;

	if ((fd = fs_open(cwd, MODE_RDONLY)) < 0) {
		shprintf("ls: Could not open directory\n");
		return;
	}

	while (1) {
		ev = fs_read(fd, (char *)&de, sizeof(struct dirent));
		if (ev < 0)
			shprintf(" : error occured.\n");
		else if (ev == 0)
			break;
		else
			shprintf("%s %d\n", de.name, de.inode);
	}

	if ((ev = fs_close(fd)) < 0)
		shprintf(" : error occured.\n");
}

/* cat */
static void cat(char *filename) {
	int fd, ev;
	char buf[SHELL_SIZEX * 3 + 1];

	if ((fd = fs_open(filename, MODE_WRONLY | MODE_CREAT | MODE_TRUNC)) < 0) {
		shprintf("Could not open file\n");
		return;
	}

	while (1) {
		shprintf("cat>");
		read_line(SHELL_SIZEX * 3, buf);
		if (same_string(buf, "."))
			break;
		if ((ev = fs_write(fd, buf, strlen(buf))) < 0) {
			shprintf("cat> fs_write error\n");
			break;
		}
	}

	fs_close(fd);
}

/* more */
static void more(char *filename) {
	int fd, read, ev;
	char buf[BLOCK_SIZE];

	if ((fd = fs_open(filename, MODE_RDONLY)) < 0) {
		shprintf("more> Could not open file\n");
		return;
	}

	while (1) {
		read = fs_read(fd, buf, BLOCK_SIZE);
		if (read < 0)
			shprintf(" : error occured.\n");
		else if (read == 0)
			break;
		buf[read] = '\0';
		shprintf("%s\n", buf);
	}

	if ((ev = fs_close(fd)) < 0)
		shprintf(" : error occured.\n");
}

/* Return the status information about a file */
static void stat(char *filename) {
	int fd, size, ev;
	char buf[STAT_SIZE], type, refs;

	if ((fd = fs_open(filename, MODE_RDONLY)) == -1) {
		shprintf("fs_open error\n");
		return;
	}

	if ((ev = fs_stat(fd, buf)) < 0)
		shprintf(" : error occured.");

	type = buf[0];
	refs = buf[1];
	bcopy(&buf[2], (char *)&size, sizeof(int));

	shprintf("filename: %s\n", filename);
	shprintf("type: %d\n", type);
	shprintf("references: %d\n", refs);
	shprintf("size: %d\n", size);

	if ((ev = fs_close(fd)) < 0)
		shprintf(" : error occured.\n");
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
