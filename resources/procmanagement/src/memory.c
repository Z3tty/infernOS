/*
 * PHYSICAL memory allocation
 * Implemented as a monitor so only one process/thread can allocate
 * or free memory at any time.
 */

#include "kernel.h"
#include "memory.h"
#include "scheduler.h"
#include "thread.h"
#include "util.h"

/* spinlock to control the access to memory allocation */
static spinlock_t memory_lock;

static uint32_t next_free_mem;

/*
 * Called by _start() Must be called before we can allocate memory,
 * and before we call any "paging" function.
 */
void init_memory(void) {
	next_free_mem = MEM_START;
	spinlock_init(&memory_lock);
}

/*
 * Allocate size bytes of physical memory. This function is called by
 * allocate_page() to allocate memory for a page directory or a page
 * table,and by loadproc() to allocate memory for code + data + user
 * stack.
 *
 * Note: if size < 4096 bytes, then 4096 bytes are used, beacuse the
 * memory blocks must be aligned to a page boundary.
 */
uint32_t alloc_memory(uint32_t size) {
	uint32_t ptr;

	spinlock_acquire(&memory_lock);
	ptr = next_free_mem;
	next_free_mem += size;

	if ((next_free_mem & 0xfff) != 0) {
		/* align next_free_mem to page boundary */
		next_free_mem = (next_free_mem & 0xfffff000) + 0x1000;
	}
#ifdef DEBUG
	scrprintf(11, 1, "%x", next_free_mem);
	scrprintf(12, 1, "%x", MEM_END);
	scrprintf(13, 1, "%x", size);
#endif /* DEBUG */
	ASSERT2(next_free_mem < MEM_END, "Memory exhausted!");
	spinlock_release(&memory_lock);
	return ptr;
}

void free_memory(uint32_t ptr) {
	spinlock_acquire(&memory_lock);

	/* do nothing now, we are not reclaiming */

	spinlock_release(&memory_lock);
}
