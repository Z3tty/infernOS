/*
 * This code currently has nothing to do with the process of paging to
 * disk. It only contains code to set up a page directory and page
 * table for a new process and for the kernel and kernel threads. The paging
 * mechanism is used for protection.
 *
 * Note:
 * This code assumes that the kernel can be mapped fully within the first
 * page table, and that any process only spans a single page table (4 MB).
 *
 * --------------------------------------
 * Virtual memory using page directories and tables is documented in
 * Protected Mode Software Architecture Chapter 13.
 *
 * A virtual memory address is split into:
 * Bits 31-22 are the page group bits (index into page directory)
 * Bits 21-12 are the page address bits (index into page table)
 * Bits 11-0  are the address within the page.
 *
 *
 * Register CR3 contains:
 * B 31-11 : page directory base address.
 * B 10-5  : reserved
 * B 4     : PCD (Page Cache Disable)
 * B 3     : PWT (Page Write Through)
 * B 2-0   : Reserved
 *
 * Paging enabled by setting CR0[PE] to one.
 * CR0:
 * 31      : Paging Enable
 * 30      : Cache Disable
 * 29      : Not Write-Through
 * 18      : Alignment Mask
 * 16      : Write Protect
 * 5       : Numeric Error Enable
 * 4       : Extension Type
 * 3       : Task Switched
 * 2       : Emulate Numeric Extension
 * 1       : Math Present
 * 0       : Protected Mode Enable
 *
 * Page Directory Entry:
 * 31-12   : Upper 20 bit of page table address
 * 11-09   : available (for user)
 *     8   : 0
 *     7   : Page Size (0=4KB, 1=4MB)
 *           If 4MB, 31:22 is physical start of 4MB page, Bit 4 of CR4 must be
 *           set to enable 4MB pages.
 *     6   : 0
 *     5   : Accessed bit
 *     4   : Page Cache Disable (0 = enable, 1 = disable)
 *     3   : Page Write Through (0 = write back, 1 = write through)
 *     2   : User/Supervisor bit (0 = OS, 1 = Apps)
 *     1   : Read/Write bit (0 = read only, 1 = read/write)
 *     0   : Page Present bit (1 = page present)
 *           if page not present, bits 31-1 can be used to store user data
 *           (addr on mass storage etc)
 *
 * Page table entry:
 * 31-12   : Page base address
 * 11-09   : available
 * 08-07   : 0
 *     6   : Dirty bit
 *     5   : Accessed bit
 *     4   : Page Cache Disable
 *     3   : Page Write-Through
 *     2   : User/Supervisor bit (0 = OS, 1 = Apps)
 *     1   : Read/Write bit (0 = read only, 1 = read/write)
 *     0   : Page Present bit (1 = page present)
 *
 */

#include "kernel.h"
#include "memory.h"
#include "paging.h"
#include "scheduler.h"
#include "thread.h"
#include "util.h"

/*
 * The kernel's page directory, which is shared with the kernel
 * threads. The first call to setup_page_table allocates and
 * initializes this.
 */
static uint32_t *kernel_page_directory = NULL;

/* Allocate a page of memory which is nulled out (overwritten by zeroes). */
static uint32_t *allocate_page(void);

/*
 * The lock is acquired and released in make_page_directory(). It is
 * initialized in make_kernel_page_directory() which is called from
 * _start()
 */
static lock_t paging_lock;

/* Use virtual address to get index in page directory. */
inline int get_directory_index(uint32_t vaddr) {
	return (vaddr >> PAGE_DIRECTORY_BITS) & PAGE_TABLE_MASK;
}

/*
 * Use virtual address to get index in a page table. The bits are
 * masked, so we essentially get get a modulo 1024 index. The
 * selection of which page table to index into is done with
 * get_directory_index().
 */
inline int get_table_index(uint32_t vaddr) {
	return (vaddr >> PAGE_TABLE_BITS) & PAGE_TABLE_MASK;
}

/*
 * Maps a page as present in the page table.
 *
 * 'vaddr' is the virtual address which is mapped to the physical
 * address 'paddr'.
 *
 * If user is nonzero, the page is mapped as accessible from a user
 * application.
 */
inline void table_map_present(uint32_t *table, uint32_t vaddr, uint32_t paddr, int user) {
	int access = RW | PRESENT, index = get_table_index(vaddr);

	if (user)
		access |= USER;

	table[index] = (paddr & ~PAGE_MASK) | access;
}

/*
 * Make an entry in the page directory pointing to the given page
 * table.  vaddr is the virtual address the page table start with
 * table is the physical address of the page table
 *
 * If user is nonzero, the page is mapped as accessible from a user
 * application.
 */
inline void directory_insert_table(uint32_t *directory, uint32_t vaddr, uint32_t *table, int user) {
	int access = RW | PRESENT, index = get_directory_index(vaddr);
	uint32_t taddr;

	if (user)
		access |= USER;

	taddr = (uint32_t)table;

	directory[index] = (taddr & ~PAGE_MASK) | access;
}

/*
 * This sets up mapping for memory that should be shared between the
 * kernel and the user process. We need this since an interrupt or
 * exception doesn't change to another page directory, and we need to
 * have the kernel mapped in to handle the interrupts. So essentially
 * the kernel needs to be mapped into the user process address space.
 *
 * The user process can't access the kernel internals though, since
 * the processor checks the privilege level set on the pages and we
 * only set USER privileges on the pages the user process should be
 * allowed to access.
 *
 * Note:
 * - we identity map the pages, so that physical address is
 *   the same as the virtual address.
 *
 * - The user processes need access video memory directly, so we set
 *   the USER bit for the video page if we make this map in a user
 *   directory.
 */
static void make_common_map(uint32_t *page_directory, int user) {
	uint32_t *page_table, addr;

	/* Allocate memory for the page table  */
	page_table = allocate_page();

	/* Identity map the first 640KB of base memory */
	for (addr = 0; addr < 640 * 1024; addr += PAGE_SIZE)
		table_map_present(page_table, addr, addr, 0);

	/* Identity map the video memory, from 0xb8000-0xb8fff. */
	table_map_present(page_table, (uint32_t)SCREEN_ADDR, (uint32_t)SCREEN_ADDR, user);

	/*
	 * Identity map in the rest of the physical memory so the
	 * kernel can access everything in memory directly.
	 */
	for (addr = MEM_START; addr < MEM_END; addr += PAGE_SIZE)
		table_map_present(page_table, addr, addr, 0);

	/*
	 * Insert in page_directory an entry for virtual address 0
	 * that points to physical address of page_table.
	 */
	directory_insert_table(page_directory, 0, page_table, user);
}

/*
 * This function is called from _start(). It is a initialization
 * function. It sets up the page directory for the kernel and kernel
 * threads. Note that it also initializes the paging_lock.
 */
void make_kernel_page_directory(void) {
	lock_init(&paging_lock);

	/*
	 * Allocate memory for the page directory. A page directory
	 * is exactly the size of one page.
	 */
	kernel_page_directory = allocate_page();

	/* This takes care of all the mapping that the kernel needs  */
	make_common_map(kernel_page_directory, 0);
}

/*
 * Returns a pointer to a freshly allocated page in physical
 * memory. The address is aligned on a page boundary.  The page is
 * zeroed out before the pointer is returned.
 */
static uint32_t *allocate_page(void) {
	uint32_t *page = (uint32_t *)alloc_memory(PAGE_SIZE);
	int i;

	scrprintf(23, 70, "%x", (uint32_t)page);

	for (i = 0; i < 1024; page[i++] = 0)
		;

	return page;
}

/*
 * Allocates and sets up the page directory and any necessary page
 * tables for a given process or thread.
 *
 * Note: We assume that the user code and data for the process fits
 * within a single page table (though not the same as the one the
 * kernel is mapped into). This works for project 4.
 */
void make_page_directory(pcb_t *proc) {
	uint32_t *page_directory, *page_table, offset, paddr, vaddr;

	lock_acquire(&paging_lock);

	if (proc->is_thread) {
		/*
		 * Threads use the kernels page directory, so just set
		 * a pointer to that one and return.
		 */
		proc->page_directory = kernel_page_directory;
		lock_release(&paging_lock);
		return;
	}
	/*
	 * User process. Allocate memory for page directory map in the
	 * address space we share with the kernel.
	 */
	page_directory = allocate_page();
	make_common_map(page_directory, 1);

	/* Now we need to set up for the user processes address space. */
	page_table = allocate_page();
	directory_insert_table(page_directory, PROCESS_START, page_table, 1);

	/*
	 * Map in the image that we loaded in from disk. limit is the
	 * size of the process image + stack in bytes
	 */
	for (offset = 0; offset < proc->limit; offset += PAGE_SIZE) {
		paddr = proc->base + offset;
		vaddr = PROCESS_START + offset;
		table_map_present(page_table, vaddr, paddr, 1);
	}
	proc->page_directory = page_directory;

	lock_release(&paging_lock);
}
