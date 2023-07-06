/*
 * File system error codes, and a function to print them.
 */

#ifdef LINUX_SIM

#include "fs_error.h"
#include <stdio.h>

#else

#include "fs_error.h"
#include "util.h"

#endif

struct fse et[] = {
    //  {FSE_MAX, "123456789012345678901234567890"},
    {FSE_OK, "No error"},
    {FSE_ERROR, "Unspecified"},
    /* File system is inconsistent */
    {FSE_FSINC, "File system is inconsistent"},
    /* Invalid mode */
    {FSE_INVALIDMODE, "Invalid mode"},
    /* Filename is to long */
    {FSE_NAMETOLONG, "Filename is to long"},
    /* File does not exist */
    {FSE_NOTEXIST, "File does not exist"},
    /* Invalid file handle */
    {FSE_INVALIDHANDLE, "Invalig file handle"},
    /* Invalid offset */
    {FSE_INVALIDOFFSET, "Invalid offset"},
    /* End of file reached */
    {FSE_EOF, "End of file"},
    /* File already exist */
    {FSE_EXIST, "A file with that name already exist"},
    /* Invalid filename */
    {FSE_INVALIDNAME, "Invalid filename"},
    /* Directory is not empty */
    {FSE_DNOTEMPTY, "Directory is not empty"},
    /* Invalid inode */
    {FSE_INVALIDINODE, "Inode error"},
    /* Out of file descriptor table entrys */
    {FSE_NOMOREFDTE, "Out of file descriptor table enetries"},
    /* Bitmap error */
    {FSE_BITMAP, "Bitmap error"},
    /* Out of inodes */
    {FSE_NOMOREINODES, "Out of inodes"},
    /* Out of datablocks */
    {FSE_FULL, "Out of data blocks"},
    /* Could not add directory entry */
    {FSE_ADDDIR, "Could not add directory entry"},
    /* Direcory entry was not found */
    {FSE_DENOTFOUND, "Directory entry not found"},
    /* A given direcrtory path is actually a file */
    {FSE_DIRISFILE, "Directory is a file"},
    /* Parse filename error */
    {FSE_PARSEERROR, "Parse filename error"},
    /* Inode table full */
    {FSE_INODETABLEFULL, "Inode table full"},
    /* Invalid block */
    {FSE_INVALIDBLOCK, "Inode contains invalid block pointer"},
    /* Tried to delete a file that was opened by another program */
    {FSE_FILEOPEN, "File to delete is used by another program"}};

#ifdef LINUX_SIM

void print_fse(int error) {
	if (error >= 0) {
		printf("%s\n", et[0].msg);
	}
	else if (error > FSE_COUNT) {
		printf("%s\n", et[error * -1].msg);
	}
	else {
		printf("Invalid error number\n");
	}
}

#else

#define FSE_LINE 0
#define FSE_COL 0

void print_fse(int error) {
	if (error >= 0) {
		scrprintf(FSE_LINE, FSE_COL, et[0].msg);
	}
	else if (error > FSE_COUNT) {
		scrprintf(FSE_LINE, FSE_COL, et[error * -1].msg);
	}
	else {
		scrprintf(FSE_LINE, FSE_COL, "Invalid error number\n");
	}
}

#endif
