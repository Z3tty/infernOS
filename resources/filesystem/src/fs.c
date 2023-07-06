#include "fs.h"

#ifdef LINUX_SIM
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#endif /* LINUX_SIM */

#include "block.h"
#include "common.h"
#include "fs_error.h"
#include "inode.h"
#include "kernel.h"
#include "superblock.h"
#include "thread.h"
#include "util.h"

#define BITMAP_ENTRIES 256

#define INODE_TABLE_ENTRIES 20

static char inode_bmap[BITMAP_ENTRIES];
static char dblk_bmap[BITMAP_ENTRIES];

static int get_free_entry(unsigned char *bitmap);
static int free_bitmap_entry(int entry, unsigned char *bitmap);
static inode_t name2inode(char *name);
static blknum_t ino2blk(inode_t ino);
static blknum_t idx2blk(int index);

/*
 * Exported functions.
 */
void fs_init(void) {

	block_init();
}

/*
 * Make a new file system.
 * Argument: kernel size
 */
void fs_mkfs(void) {
}

int fs_open(const char *filename, int mode) {
	return -1;
}

int fs_close(int fd) {
	return -1;
}

int fs_read(int fd, char *buffer, int size) {
	return -1;
}

int fs_write(int fd, char *buffer, int size) {
	return -1;
}

/*
 * fs_lseek:
 * This function is really incorrectly named, since neither its offset
 * argument or its return value are longs (or off_t's).
 */
int fs_lseek(int fd, int offset, int whence) {
	return -1;
}

int fs_mkdir(char *dirname) {
	return -1;
}

int fs_chdir(char *path) {
	return -1;
}

int fs_rmdir(char *path) {
	return -1;
}

int fs_link(char *linkname, char *filename) {
	return -1;
}

int fs_unlink(char *linkname) {

	return -1;
}

int fs_stat(int fd, char *buffer) {
	return -1;
}

/*
 * Helper functions for the system calls
 */

/*
 * get_free_entry:
 *
 * Search the given bitmap for the first zero bit.  If an entry is
 * found it is set to one and the entry number is returned.  Returns
 * -1 if all entrys in the bitmap are set.
 */
static int get_free_entry(unsigned char *bitmap) {
	int i;

	/* Seach for a free entry */
	for (i = 0; i < BITMAP_ENTRIES / 8; i++) {
		if (bitmap[i] == 0xff) /* All taken */
			continue;
		if ((bitmap[i] & 0x80) == 0) { /* msb */
			bitmap[i] |= 0x80;
			return i * 8;
		}
		else if ((bitmap[i] & 0x40) == 0) {
			bitmap[i] |= 0x40;
			return i * 8 + 1;
		}
		else if ((bitmap[i] & 0x20) == 0) {
			bitmap[i] |= 0x20;
			return i * 8 + 2;
		}
		else if ((bitmap[i] & 0x10) == 0) {
			bitmap[i] |= 0x10;
			return i * 8 + 3;
		}
		else if ((bitmap[i] & 0x08) == 0) {
			bitmap[i] |= 0x08;
			return i * 8 + 4;
		}
		else if ((bitmap[i] & 0x04) == 0) {
			bitmap[i] |= 0x04;
			return i * 8 + 5;
		}
		else if ((bitmap[i] & 0x02) == 0) {
			bitmap[i] |= 0x02;
			return i * 8 + 6;
		}
		else if ((bitmap[i] & 0x01) == 0) { /* lsb */
			bitmap[i] |= 0x01;
			return i * 8 + 7;
		}
	}
	return -1;
}

/*
 * free_bitmap_entry:
 *
 * Free a bitmap entry, if the entry is not found -1 is returned, otherwise zero.
 * Note that this function does not check if the bitmap entry was used (freeing
 * an unused entry has no effect).
 */
static int free_bitmap_entry(int entry, unsigned char *bitmap) {
	unsigned char *bme;

	if (entry >= BITMAP_ENTRIES)
		return -1;

	bme = &bitmap[entry / 8];

	switch (entry % 8) {
	case 0:
		*bme &= ~0x80;
		break;
	case 1:
		*bme &= ~0x40;
		break;
	case 2:
		*bme &= ~0x20;
		break;
	case 3:
		*bme &= ~0x10;
		break;
	case 4:
		*bme &= ~0x08;
		break;
	case 5:
		*bme &= ~0x04;
		break;
	case 6:
		*bme &= ~0x02;
		break;
	case 7:
		*bme &= ~0x01;
		break;
	}

	return 0;
}

/*
 * ino2blk:
 * Returns the filesystem block (block number relative to the super
 * block) corresponding to the inode number passed.
 */
static blknum_t ino2blk(inode_t ino) {
	return (blknum_t)-1;
}

/*
 * idx2blk:
 * Returns the filesystem block (block number relative to the super
 * block) corresponding to the data block index passed.
 */
static blknum_t idx2blk(int index) {
	return (blknum_t)-1;
}

/*
 * name2inode:
 * Parses a file name and returns the corresponding inode number. If
 * the file cannot be found, -1 is returned.
 */
static inode_t name2inode(char *name) {
	return (inode_t)-1;
}
