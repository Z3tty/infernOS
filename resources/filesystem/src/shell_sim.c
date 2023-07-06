/* Fake shell */

/*
 * TODO: Directory/filename length
 */

#include <stdio.h>
#include <stdlib.h>

#include "block.h"
#include "fs.h"
#include "kernel.h"
#include "util.h"

#define SIZEX 50

struct pcb fake_pcb;
struct pcb *current_running = &fake_pcb;

/* Parse command in 'line'. Arguments are stored in argv. Returns
 * number of arguments */
static int parse_line(char *line, char *argv[SIZEX]);
/* Add to cwd the path in 'path'. */
static void change_cwd(char *cwd, char *path);
/* Print strings command and usage */
static void usage(char *command, char *usage);
/* Print file system error value */
static void print_fse(int ev);

/* Shell commands */
static void ls(char *path);
static void cat(char *filename);
static void more(char *filename);
static void stat(char *filename);

int os_size = 0;

int main(void) {
	int ev;
	int argc;                  /* argument count */
	char *argv[SIZEX];         /* argument vector */
	char line[SIZEX + 1];      /* unparsed command */
	char cwd[SIZEX + 1] = "/"; /* current working directory */

	/* Initalize various subsystems */
	fs_init();

	while (1) {
		printf("%s", cwd);
		printf("$ ");
		fgets(line, SIZEX, stdin);
		fflush(stdin);
		line[strlen(line) - 1] = '\0'; /* Remove \n */

		argc = parse_line(line, argv);

		/* if no arguments goto beginning of loop */
		if (argc == 0) {
			continue;
		}

		if (same_string("mkdir", argv[0])) {
			if (argc == 2) {
				if ((ev = fs_mkdir(argv[1])) < 0)
					print_fse(ev);
			}
			else {
				usage(argv[0], " 'directory name'");
				continue;
			}
		}
		else if (same_string("cd", argv[0])) {
			if (argc == 2) {
				if ((ev = fs_chdir(argv[1])) < 0)
					print_fse(ev);
				else
					change_cwd(cwd, argv[1]);
			}
			else {
				usage(argv[0], " 'directory name'");
				continue;
			}
		}
		else if (same_string("rmdir", argv[0])) {
			if (argc == 2) {
				if ((ev = fs_rmdir(argv[1])) < 0)
					print_fse(ev);
			}
			else {
				usage(argv[0], " 'directory name'");
				continue;
			}
		}
		else if (same_string("ls", argv[0])) {
			if (argc == 1) {
				ls(cwd);
			}
			else {
				usage(argv[0], "");
				continue;
			}
		}
		else if (same_string("pwd", argv[0])) {
			if (argc == 1) {
				printf("%s\n", cwd);
			}
			else {
				usage(argv[0], "");
				continue;
			}
		}
		else if (same_string("cat", argv[0])) {
			if (argc == 2) {
				cat(argv[1]);
			}
			else {
				usage(argv[0], " 'file name'");
				continue;
			}
		}
		else if (same_string("more", argv[0])) {
			if (argc == 2) {
				more(argv[1]);
			}
			else {
				usage(argv[0], " 'file name'");
				continue;
			}
		}
		else if (same_string("ln", argv[0])) {
			if (argc == 3) {
				if ((ev = fs_link(argv[1], argv[2])) < 0)
					print_fse(ev);
			}
			else {
				usage(argv[0], " 'link name' 'file name' ");
				continue;
			}
		}
		else if (same_string("rm", argv[0])) {
			if (argc == 2) {
				if ((ev = fs_unlink(argv[1])) < 0)
					print_fse(ev);
			}
			else {
				usage(argv[0], " 'file name'");
				continue;
			}
		}
		else if (same_string("stat", argv[0])) {
			if (argc == 2) {
				stat(argv[1]);
			}
			else {
				usage(argv[0], " 'file name'");
				continue;
			}
		}
		else if (same_string("exit", argv[0])) {
			if (argc == 1) {
				block_destruct();
				return 0;
			}
			else {
				usage(argv[0], " 'file name'");
			}
		}
		else {
			printf("Unknown command\n");
		}
	}

	return 0;
}

/* Print strings command and usage */
static void usage(char *command, char *usage) {
	printf("Usage: %s %s\n", command, usage);
}

/* Parse command in 'line'.
 * Pointers to arguments (in line) are stored in argv.
 * Returns number of arguments.
 */
static int parse_line(char *line, char *argv[SIZEX]) {
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

/* Extract every directory name out of a path. This consist of replacing every /
 * with '\0' */
static int parse_path(char *path, char *argv[SIZEX]) {
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
	char *argv[SIZEX];
	int argc, i;
	char *s, *p;

	if (*path == '/') { /* is absoulte */
		strcpy(cwd, path);
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

	printf("dirs: \n");
	for (i = 0; i < argc; i++) {
		printf("\t%s\n", argv[i]);
	}

	for (i = 0; i < argc; i++) {
		if (same_string(argv[i], ".")) {
			; /* do nothing */
		}
		else if (same_string(argv[i], "..")) { /* Remove last part of the
			                                       * filename */
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

/* ls - print out a unsorted list of filenames.
 * TODO: sort files.
 */
static void ls(char *cwd) {
	int fd, ev;
	struct dirent de;

	if ((fd = fs_open(cwd, MODE_RDONLY)) < 0) {
		printf("ls: Could not open directory\n");
		print_fse(fd);
		return;
	}

	while (1) {
		ev = fs_read(fd, (char *)&de, sizeof(struct dirent));
		if (ev < 0)
			print_fse(ev);
		else if (ev == 0)
			break;
		else
			printf("\t%s %d\n", de.name, de.inode);
	}

	if ((ev = fs_close(fd)) < 0)
		print_fse(ev);
}

/* cat */
static void cat(char *filename) {
	int fd, ev;
	char buf[SIZEX];

	if ((fd = fs_open(filename, MODE_WRONLY | MODE_CREAT | MODE_TRUNC)) < 0) {
		printf("cat> Could not open file %s\n", filename);
		print_fse(fd);
		return;
	}

	while (1) {
		printf("cat>");
		fgets(buf, SIZEX, stdin);
		fflush(stdin);
		if (same_string(buf, ".\n"))
			break;
		if ((ev = fs_write(fd, buf, strlen(buf))) < 0) {
			printf("cat> fs_write error\n");
			print_fse(ev);
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
		printf("more> Could not open file %s\n", filename);
		print_fse(fd);
		return;
	}

	while (1) {
		read = fs_read(fd, buf, BLOCK_SIZE);
		if (read < 0)
			print_fse(read);
		else if (read == 0)
			break;
		buf[read] = '\0';
		printf("more> %s\n", buf);
	}

	if ((ev = fs_close(fd)) < 0)
		print_fse(ev);
}

/* Return the status information about a file */
static void stat(char *filename) {
	int fd, size, ev;
	char buf[STAT_SIZE], type, refs;

	if ((fd = fs_open(filename, MODE_RDONLY)) == -1) {
		printf("fs_open error\n");
		print_fse(fd);
		return;
	}

	if ((ev = fs_stat(fd, buf)) < 0)
		print_fse(ev);

	type = buf[0];
	refs = buf[1];
	bcopy(&buf[2], (char *)&size, sizeof(int));
	printf("stat\n"
	       "filename: type, refs, size\n");
	printf("%s: %d %d %d\n", filename, type, refs, size);

	if ((ev = fs_close(fd)) < 0)
		print_fse(ev);
}

/* Print file system error value */
static void print_fse(int ev) {
	printf("File system error value: %d\n", ev);
}
