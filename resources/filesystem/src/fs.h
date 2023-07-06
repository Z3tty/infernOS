#ifndef FS_H
#define FS_H

#include "block.h"
#include "fstypes.h"

/*
 * type for inode number. This is just the offset from the start of
 * the inode area on disk.
 */
#ifndef LINUX_SIM

#endif /* LINUX_SIM */

/* Number of file system blocks.
 *
 * If you decide to change this, you must probably change the number
 * of blocks reserved for the filesystem in createimage.c as well.*/
#define FS_BLOCKS (512 + 2)

#define MASK(v) (1 << (v))

/* fs_open mode flags */

/* This mode is used to mark a file descriptor table as unused */
#define MODE_UNUSED 0

/* Exactly one of these modes must be specified in fs_open */

#define MODE_RDONLY_BIT 1 /* Open for reading only */
#define MODE_RDONLY MASK(MODE_RDONLY_BIT)

#define MODE_WRONLY_BIT 2 /* Open for writing only */
#define MODE_WRONLY MASK(MODE_WRONLY_BIT)

#define MODE_RDWR_BIT 3 /* Open for reading and writing */
#define MODE_RDWR MASK(MODE_RDWR_BIT)

#define MODE_RWFLAGS_MASK (MODE_RDONLY | MODE_WRONLY | MODE_RDWR)

/* Any combination of the modes below are legal */

/* Makes no sense to have MODE_RDONLY together with this one */
#define MODE_CREAT_BIT 4 /* Create the file if it doesn't exist */
#define MODE_CREAT MASK(MODE_CREAT_BIT)

/* Makes no sense to have MODE_RDONLY together with this one */
#define MODE_TRUNC_BIT 5 /* Set file size to 0 */
#define MODE_TRUNC MASK(MODE_TRUNC_BIT)

enum
{
	MAX_FILENAME_LEN = 14,
	MAX_PATH_LEN = 256, /* Total length of a path */
	STAT_SIZE = 6,      /* Size of the information returned by fs_stat */
};

/* A directory entry */
struct dirent {
	inode_t inode;
	char name[MAX_FILENAME_LEN];
};

#define DIRENTS_PER_BLK (BLOCK_SIZE / sizeof(struct dirent))

#ifndef SEEK_SET
enum
{
	SEEK_SET,
	SEEK_CUR,
	SEEK_END
};
#endif

void fs_init(void);
void fs_mkfs(void);
int fs_open(const char *filename, int mode);
int fs_close(int fd);
int fs_read(int fd, char *buffer, int size);
int fs_write(int fd, char *buffer, int size);
int fs_lseek(int fd, int offset, int whence);
int fs_link(char *linkname, char *filename);
int fs_unlink(char *linkname);
int fs_stat(int fd, char *buffer);

int fs_mkdir(char *dir_name);
int fs_chdir(char *path);
int fs_rmdir(char *path);

#endif
