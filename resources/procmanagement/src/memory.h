#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

enum
{
	/*
	 * MEM_END <= 400000 (4MB), because we use _one_ page table to
	 * identity map memory[0..MEM_END] in paging.c, make_common_map()
	 *
	 * Process memory: between 1 and 2M
	 */
	MEM_START = 0x100000,
	MEM_END = 0x200000
};

/*
 * Must be called before we can allocate memory, and before we call
 * any "paging" function. (is called by _start())
 */
void init_memory(void);

/* Allocate size bytes of mmeory aligned to a page boudary */
uint32_t alloc_memory(uint32_t size);

/* Not implemented. Should free the block pointed to by <ptr> */
void free_memory(uint32_t ptr);

#endif /* MEMORY_H */
