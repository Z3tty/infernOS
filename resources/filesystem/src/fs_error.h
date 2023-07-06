#ifndef FS_ERROR_H
#define FS_ERROR_H

enum
{
	FSE_OK = 0,
	/* Unspecified error */
	FSE_ERROR = -1,
	/* File system is inconsistent */
	FSE_FSINC = -2,
	/* Invalid mode */
	FSE_INVALIDMODE = -3,
	/* Filename is to long */
	FSE_NAMETOLONG = -4,
	/* File does not exist */
	FSE_NOTEXIST = -5,
	/* Invalid file handle */
	FSE_INVALIDHANDLE = -6,
	/* Invalid offset */
	FSE_INVALIDOFFSET = -7,
	/* End of file reached */
	FSE_EOF = -8,
	/* File already exist */
	FSE_EXIST = -9,
	/* Invalid filename */
	FSE_INVALIDNAME = -10,
	/* Directory is not empty */
	FSE_DNOTEMPTY = -11,
	/* Invalid inode */
	FSE_INVALIDINODE = -12,
	/* Out of file descriptor table entrys */
	FSE_NOMOREFDTE = -13,
	/* Bitmap error */
	FSE_BITMAP = -14,
	/* Out of inodes */
	FSE_NOMOREINODES = -15,
	/* Out of datablocks */
	FSE_FULL = -16,
	/* Could not add directory entry */
	FSE_ADDDIR = -17,
	/* Direcory entry was not found */
	FSE_DENOTFOUND = -18,
	/* A given direcrtory path is actually a file */
	FSE_DIRISFILE = -19,
	/* Parse filename error */
	FSE_PARSEERROR = -20,
	/* Inode table full */
	FSE_INODETABLEFULL = -21,
	/* Invalid block */
	FSE_INVALIDBLOCK = -22,
	/* Tried to delete a file that was opened by another program */
	FSE_FILEOPEN = -23,
	FSE_COUNT = -24
};

enum
{
	MAX_ERROR_LENGTH = 50
};

struct fse {
	int error;
	char msg[MAX_ERROR_LENGTH];
};

void print_fse(int error);

#endif
