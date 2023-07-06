/* Header file for block.c and blockFake.c */

#ifndef BLOCK_H
#define BLOCK_H

#include "common.h"

#define BLOCK_SIZE SECTOR_SIZE
#define BLOCKS (SECTORS / (BLOCK_SIZE / SECTOR_SIZE))

void block_init(void);
void block_destruct(void);
int block_read(int block_num, void *address);
int block_write(int block_num, void *address);
int block_modify(int block_num, int offset, void *data, int data_size);
int block_read_part(int block_num, int offset, int bytes, void *address);

#endif /* !BLOCK_H */
