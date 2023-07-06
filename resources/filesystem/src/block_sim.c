/*
 * This simulates the operation of the filesystem on Linux.
 *
 * The block function read or writes a block of the file system.
 */

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "block.h"
#include "util.h"

static FILE *fp; /* The file used to simulate a diskette */

static void error(char *fmt, ...);

/* Initialize the file */
void block_init(void) {
	if ((fp = fopen("image_sim", "r+")) == NULL) {
		error("could not open image file:");
	}
}

void block_destruct(void) {
	fclose(fp);
}

/* Read a block into memory[address] */
int block_read(int block_num, void *address) {
	if (fseek(fp, block_num * BLOCK_SIZE, SEEK_SET) < 0) {
		error("fseek error: ");
	}

	if (fread(address, BLOCK_SIZE, 1, fp) != 1) {
		error("fread error: ");
	}
#ifndef NDEBUG
	printf("block %d read\n", block_num);
#endif /* NDEBUG */

	return 1;
}

/* Wrtie from memory['address'] into block 'block' in the file */
int block_write(int block_num, void *address) {
	if (fseek(fp, block_num * BLOCK_SIZE, SEEK_SET) < 0) {
		error("fseek error: ");
	}

	if (fwrite(address, BLOCK_SIZE, 1, fp) != 1) {
		error("write error: ");
	}
#ifndef NDEBUG
	printf("block %d written\n", block_num);
#endif /* NDEBUG */

	fflush(fp);
	return 1;
}

/* Modify a block */
int block_modify(int block_num, int offset, void *data, int data_size) {
	char buf[BLOCK_SIZE];

	assert((offset + data_size) <= BLOCK_SIZE);

	block_read(block_num, buf);
	bcopy(data, &buf[offset], data_size);
	block_write(block_num, buf);

#ifndef NDEBUG
	printf("block %d modified (%d bytes)\n", block_num, data_size);
#endif /* NDEBUG */

	return 1;
}

/* Read part of a block */
int block_read_part(int block_num, int offset, int bytes, void *address) {
	char buf[BLOCK_SIZE];

	assert((offset + bytes) <= BLOCK_SIZE);

	block_read(block_num, buf);
	bcopy(&(buf[offset]), address, bytes);

#ifndef NDEBUG
	printf("block %d read from %d (%d bytes)\n", block_num, offset, bytes);
#endif /* NDEBUG */

	return 1;
}

/* print an error message and exit */
static void error(char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	if (errno != 0) {
		perror(NULL);
	}
	exit(EXIT_FAILURE);
}
