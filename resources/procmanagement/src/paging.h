#ifndef PAGING_H
#define PAGING_H

#include "kernel.h"

enum
{
	/*
	 * page directory/table entry bits (PMSA p.235 and p.240)
	 */
	PRESENT = 1,        /* present */
	RW = 2,             /* read/ write */
	USER = 4,           /* user/ supervisor */
	WRITE_THROUGH = 8,  /* page write-through */
	CACHE_DISABLE = 16, /* page cache disable */
	ACCESSED = 32,      /* accessed */
	DIRTY = 64,         /* dirty */

	/* Useful sizes, bit-sizes and masks */
	PAGE_DIRECTORY_BITS = 22,         /* position of page dir index */
	PAGE_TABLE_BITS = 12,             /* position of page table index */
	PAGE_DIRECTORY_MASK = 0xffc00000, /* extracts page dir index */
	PAGE_TABLE_MASK = 0x000003ff,     /* extracts page table index */
	PAGE_SIZE = 0x1000,               /* 4096 bytes */
	PAGE_MASK = 0xfff,                /* extracts 10 lsb */

	/* size of a page table in bytes */
	PAGE_TABLE_SIZE = (1024 * 4096 - 1)
};

/*
 * Allocates and sets up the page directory and any necessary page
 * tables for a given process or thread.
 */
void make_page_directory(pcb_t *p);

/*
 * This function is called from _start(). It is a initialization
 * function. It sets up the page directory for the kernel and kernel
 * threads. It also initializes the paging_lock.
 */
void make_kernel_page_directory(void);

#endif /* !PAGING_H */
