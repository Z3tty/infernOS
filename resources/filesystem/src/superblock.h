#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H

/*
 * The superblock describes the filesystem.
 *
 * The filesystem layout looks like this:
 *
 * +-------------+-----------------------------+-/
 * | Super block |  Inode & data bitmap block  |
 * +-------------+-----------------------------+-/
 *
 * <------ Inode area ----> <-------- Data block area -------->
 * /--------+-//-+---------+--------------+-//-+--------------+
 *  Inode 1 |    | Inode n | Data block 1 |    | Data block n |
 * /--------+-//-+---------+--------------+-//-+--------------+
 *
 * The member max_filesize is:
 * BLOCK_SIZE * (NDIRECT + (BLOCK_SIZE / sizeof(blknum_t))) = 1056KB
 * at present.
 *
 * The root_inode member gives the block number on disk where the
 * inode for the root directory of this filesystem resides.
 */

#include "fstypes.h"

struct disk_superblock {
	short ninodes;       /* number of index nodes in the filesystem */
	short ndata_blks;    /* number of data blocks */
	blknum_t root_inode; /* block number of inode for the root dir */
	short max_filesize;  /* the size of the largest file */
};

#define SUPERBLK_SIZE 1
/*
 * The superblock as used in memory. The dirty member is true if
 * filesystem metadata needs to be updated (happens when one of the
 * inode bitmaps is changed).
 */

struct mem_superblock {
	struct disk_superblock d_super;
	void *ibmap; /* bitmap for inodes */
	void *dbmap; /* bitmap for data blocks */
	char dirty;
};

#endif /* SUPERBLOCK_H */
