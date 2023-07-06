/*
 * Implemented as a monitor so that only one process can get
 * its page fault handled at any time.
 *
 * Note:
 * There is no separate swap area. When a data page is swapped out,
 * it is stored in the location it was loaded from in the process'
 * image. This means it's impossible to start two processes from the
 * same image without screwing up the running. It also means the
 * disk image is read once. And that we cannot use the program disk.
 *
 * Best viewed with tabs set to 4 spaces.
 */

#include "common.h"
#include "interrupt.h"
#include "kernel.h"
#include "memory.h"
#include "scheduler.h"
#include "thread.h"
#include "usb/scsi.h"
#include "util.h"

/*
 * page_alloc allocates a page.  If necessary, it swaps a page out.
 * On success, it returns the index of the page in the page map.  On
 * failure, it aborts.  BUG: pages are not made free when a process
 * exits.
 */
static int page_alloc(int pinned);

/* page_addr returns the physical address of the i-th page */
static uint32_t *page_addr(int i);

/*
 * page_replacement_policy returns the index in the page map of a page
 * to be swapped out
 */
static int page_replacement_policy(void);

/* swap the i-th page in */
static void page_swap_in(int pageno);

/* swap the i-th page out */
static void page_swap_out(int pageno);

/* return the disk_sector of the given page */
static uint32_t page_disk_sector(page_map_entry_t *page);

/* Static global variables */
/* the page map */
static page_map_entry_t page_map[PAGEABLE_PAGES];

/* lock to control the access to the page map */
static lock_t page_map_lock;

/* address of the kernel page directory (shared by all kernel threads) */
static uint32_t *kernel_pdir;

/* addresses of the kernel page tables */
static uint32_t *kernel_pts[N_KERNEL_PTS];

/* Use virtual address to get index in page directory.  */
inline uint32_t get_directory_index(uint32_t vaddr) {
	return (vaddr & PAGE_DIRECTORY_MASK) >> PAGE_DIRECTORY_BITS;
}

/*
 * Use virtual address to get index in a page table.  The bits are
 * masked, so we essentially get a modulo 1024 index.  The selection
 * of which page table to index into is done with
 * get_directory_index().
 */
inline uint32_t get_table_index(uint32_t vaddr) {
	return (vaddr & PAGE_TABLE_MASK) >> PAGE_TABLE_BITS;
}

/* Use the virtual address to invalidate a page in the TLB. */
inline void invalidate_page(uint32_t *vaddr) {
	asm volatile("invlpg %0" : : "m"(vaddr));
}

/* Set 12 least significant bytes in a page table entry to 'mode' */
inline void page_set_mode(uint32_t *pdir, uint32_t vaddr, uint32_t mode) {
	uint32_t dir_index = get_directory_index((uint32_t)vaddr), index = get_table_index((uint32_t)vaddr), dir_entry, *table, entry;

	dir_entry = pdir[dir_index];
	ASSERT(dir_entry & PE_P); /* dir entry present */

	table = (uint32_t *)(dir_entry & PE_BASE_ADDR_MASK);
	/* clear table[index] bits 11..0 */
	entry = table[index] & PE_BASE_ADDR_MASK;

	/* set table[index] bits 11..0 */
	entry |= mode & ~PE_BASE_ADDR_MASK;
	table[index] = entry;

	/* Flush TLB */
	invalidate_page((uint32_t *)vaddr);
}

/*
 * Maps a page as present in the page table.
 *
 * 'vaddr' is the virtual address which is mapped to the physical
 * address 'paddr'. 'mode' sets bit [12..0] in the page table entry.
 *
 * If user is nonzero, the page is mapped as accessible from a user
 * application.
 */
inline void table_map_page(uint32_t *table, uint32_t vaddr, uint32_t paddr, uint32_t mode) {
	int index = get_table_index(vaddr);
	table[index] = (paddr & PE_BASE_ADDR_MASK) | (mode & ~PE_BASE_ADDR_MASK);
	invalidate_page((uint32_t *)vaddr);
}

/*
 * Insert a page table entry into the page directory.
 *
 * 'vaddr' is the virtual address which is mapped to the physical
 * address 'paddr'. 'mode' sets bit [12..0] in the page table entry.
 */
inline void dir_ins_table(uint32_t *directory, uint32_t vaddr, void *table, uint32_t mode) {
	uint32_t access = mode & MODE_MASK, taddr = (uint32_t)table;
	int index = get_directory_index(vaddr);

	directory[index] = (taddr & PE_BASE_ADDR_MASK) | access;
}

/*
 * init_memory()
 *
 * called once by _start() in kernel.c
 *
 * This function actually only sets up a page directory and page table
 * for the kernel!
 *
 * This consists of setting up N_KERNEL_PTS (one in this case) which
 * identity maps memory between 0x0 and MAX_PHYSICAL_MEMORY.
 *
 * The interrupts are off and paging is not enabled when this function
 * is called.
 */

void init_memory(void) {
	int p; /* page index in page map */
	int i, j;
	uint32_t pbaddr; /* page base address (vm) */

	/* initialize the lock to access the page map */
	lock_init(&page_map_lock);

	/* allocate the kernel page directory */
	p = page_alloc(TRUE);
	kernel_pdir = page_addr(p);

	/* for each kernel page table */
	pbaddr = 0;
	for (i = 0; i < N_KERNEL_PTS; i++) {
		/* allocate the page table */
		p = page_alloc(TRUE);
		kernel_pts[i] = page_addr(p);

		/* Insert table into the page directory */
		dir_ins_table(kernel_pdir, pbaddr, kernel_pts[i], PE_P | PE_RW);

		/* fill in the page table */
		j = 0;
		while ((pbaddr < MAX_PHYSICAL_MEMORY) && (j < PAGE_N_ENTRIES)) {
			table_map_page(kernel_pts[i], pbaddr, pbaddr, PE_P | PE_RW);
			pbaddr += PAGE_SIZE;
			j++;
		}
	}

	/* Give the user permission to write on the screen */
	page_set_mode(kernel_pdir, (uint32_t)SCREEN_ADDR, PE_P | PE_RW | PE_US);
}

/*
 * Sets up a page directory and page table for a new process or thread.
 */
void setup_page_table(pcb_t *p) {
	lock_acquire(&page_map_lock);

	if (p->is_thread) {
		/*
		 * if p is a thread, it uses the kernel page directory
		 * and page tables, and it doesn't need anything
		 * else
		 */
		p->page_directory = kernel_pdir;
	}
	else {
		/*
		 * if p is a process, we have to allocate four pages: a page
		 * directory, a page table, a stack page table, and a stack
		 * page.
		 */
		uint32_t *pde;   /* pointer to page directory entry */
		uint32_t *pte;   /* pointer to page table entry */
		int n_img_pages; /* number of pages for process image */
		int i, pdir, ptbl, stkt, stkp1, stkp2;

		/* allocate the four pages and pin them immediately */
		pdir = page_alloc(TRUE);  /* page directory */
		ptbl = page_alloc(TRUE);  /* page table */
		stkt = page_alloc(TRUE);  /* stack page table */
		stkp1 = page_alloc(TRUE); /* stack page 1 */
		stkp2 = page_alloc(TRUE); /* stack page 2 */

		/* save process page directory address */
		pde = page_addr(pdir);
		p->page_directory = pde;

		/* map kernel page tables into process page directory */
		for (i = 0; i < N_KERNEL_PTS; i++) {
			dir_ins_table(pde, PTABLE_SPAN * i, kernel_pts[i], PE_P | PE_RW | PE_US);
		}

		/* map process page table into process page directory */
		dir_ins_table(pde, PROCESS_START, page_addr(ptbl), PE_P | PE_RW | PE_US);

		/* map stack page table into process page directory */
		dir_ins_table(pde, PROCESS_STACK, page_addr(stkt), PE_P | PE_RW | PE_US);

		/* force demand paging for code and data of process */
		n_img_pages = p->swap_size / SECTORS_PER_PAGE;
		if ((p->swap_size % SECTORS_PER_PAGE) != 0)
			n_img_pages++;

		pte = page_addr(ptbl);
		for (i = 0; i < n_img_pages; i++) {
			/* set all pages to not present */
			table_map_page(pte, PROCESS_START + i * PAGE_SIZE, PROCESS_START + i * PAGE_SIZE, PE_RW | PE_US);
		}

		pte = page_addr(stkt);
		/* map two stack pages into stack page table */
		table_map_page(pte, PROCESS_STACK, (uint32_t)page_addr(stkp1), PE_P | PE_RW | PE_US);
		table_map_page(pte, PROCESS_STACK - PAGE_SIZE, (uint32_t)page_addr(stkp2), PE_P | PE_RW | PE_US);
	}

	lock_release(&page_map_lock);
}

/* Page fault but page table present and page present */
void page_protection_error(uint32_t pde, uint32_t pte) {
	uint32_t cr2 = current_running->fault_addr;
	int i = 1;

	PRINT_INFO(i, "Page protection error %s", "");
	i++;
	PRINT_INFO(i, "PID %d", current_running->pid);
	i++;
	PRINT_INFO(i, "Preemptions %d", current_running->preempt_count);
	i++;
	PRINT_INFO(i, "Yields %d", current_running->yield_count);
	i++;
	PRINT_INFO(i, "Nested count %d", current_running->nested_count);
	i++;
	PRINT_INFO(i, "Hardware mask %04x", current_running->int_controller_mask);
	i++;
	PRINT_INFO(i, "Fault address %08x", cr2);
	i++;
	PRINT_INFO(i, "Error code %08x", current_running->error_code);
	i++;
	PRINT_INFO(i, "PDirectory entry %08x", pde);
	i++;
	PRINT_INFO(i, "PTable entry %08x", pte);
	i++;
	HALT("halting due to protection error");
}

/*
 * called by exception_14 in interrupt.c (the faulting address is in
 * current_running->fault_addr)
 *
 * Interrupts are on when calling this function.
 */
void page_fault_handler(void) {
	int pdi;                /* page directory index */
	int pti;                /* page table index */
	uint32_t pde;           /* page directory entry */
	uint32_t pte;           /* page table entry */
	uint32_t *pta;          /* page table address */
	int pidx;               /* page index in page map */
	page_map_entry_t *page; /* ptr to page map entry of a page */

	current_running->page_fault_count++;
	lock_acquire(&page_map_lock);

	pdi = get_directory_index(current_running->fault_addr);
	pde = current_running->page_directory[pdi];

	/* if page table not present */
	if ((pde & PE_P) == 0) {
		/* the page fault is due to the absence of a page table */

		/* HALT("Page tables should all be pinned"); */
		page_protection_error(pde, 0);
	}
	else {
		/*
		 * The page table is present, so the page fault is due
		 * to the absence of a target page.
		 */
		scrprintf(24, 10, "%08x ", current_running->fault_addr);

		/* get page table base address from the page directory entry */
		pta = (uint32_t *)(pde & PE_BASE_ADDR_MASK);

		/* get page table index from the faulting virtual address */
		pti = get_table_index(current_running->fault_addr);

		/* get the page table entry of the faulting virtual address */
		pte = pta[pti];

		scrprintf(24, 20, "%08x ", pte);

		/* make sure the target page is indeed not present */
		if (pte & PE_P)
			page_protection_error(pde, pte);

		pidx = page_alloc(FALSE);

		/* update the mapping for the new page */
		page = &page_map[pidx];
		page->owner = current_running;
		page->swap_loc = current_running->swap_loc;
		page->swap_size = current_running->swap_size;
		page->vaddr = current_running->fault_addr & PE_BASE_ADDR_MASK;
		page->entry = &pta[pti];
		page->pinned = FALSE;

		page_swap_in(pidx);
	}
	lock_release(&page_map_lock);
}

/*
 * Allocate a page. Returns page number / index in the
 * page_map directory.
 *
 * Marks page as pinned if pinned == TRUE.
 *
 * Swaps out a page if no space is available.
 */
static int page_alloc(int pinned) {
	static int dole_ptr = 0;
	int i, page;
	uint32_t *p;

	if (dole_ptr < PAGEABLE_PAGES) {
		/* The first PAGEABLE_PAGES are trivial, hand out one by one */
		page = dole_ptr;
		dole_ptr++;
	}
	else {
		/* no free pages left: swap a page out */
		page = page_replacement_policy();
		page_swap_out(page);
	}
	ASSERT((page >= 0) && (page < PAGEABLE_PAGES));

	/* Clean out entry before returning index to it */
	page_map[page].owner = NULL;
	page_map[page].swap_loc = 0;
	page_map[page].swap_size = 0;
	page_map[page].vaddr = 0;
	page_map[page].entry = NULL;
	page_map[page].pinned = pinned;

	/* Zero out page before returning  */
	p = page_addr(page);
	for (i = 0; i < PAGE_N_ENTRIES; i++) {
		p[i] = 0;
	}
	return page;
}

/* Returns physical address of page number i */
static uint32_t *page_addr(int i) {
	if (i < 0 || i >= PAGEABLE_PAGES) {
		scrprintf(3, 0, "page: %-3d", i);
		scrprintf(4, 0, "valid: 0--%d", PAGEABLE_PAGES - 1);
		HALT("page number out of range of pageable pages");
	}
	return (uint32_t *)(MEM_START + (PAGE_SIZE * i));
}

/* Decide which page to replace, return the page number  */
static int page_replacement_policy(void) {
	static int page = -1;
	bool_t found;
	int i;

	/* check if there is any page not pinned */
	found = FALSE;
	i = 0;
	while ((!found) && (i < PAGEABLE_PAGES)) {
		i++;
		found = (page_map[i].pinned == FALSE);
	}
	ASSERT2(found, "All pages pinned");

	/*
	 * Cycle through looking for an unpinned page.  Avoid last
	 * swapped page if possible.
	 */
	while (1) {
		page++;
		if (page >= PAGEABLE_PAGES)
			page = 0;
		if (page_map[page].pinned == FALSE)
			return page;
	}
}

/* Swap page in from image */
static void page_swap_in(int pageno) {
	page_map_entry_t *page = &page_map[pageno];
	uint32_t addr = (uint32_t)page_addr(pageno);
	uint32_t sector = page_disk_sector(page), nsectors;

	scrprintf(23, 50, "pid %-3d rding page %-3d", current_running->pid, pageno);

	if ((sector + SECTORS_PER_PAGE) > (page->swap_loc + page->swap_size)) {
		/*
		 * if the final sector is past the end of the image
		 * read only the sectors that belong to this image
		 */
		nsectors = page->swap_size % SECTORS_PER_PAGE;
	}
	else {
		/* else, read an entire page */
		nsectors = SECTORS_PER_PAGE;
	}

	scsi_read(sector, nsectors, (char *)addr);
	*page->entry = PE_P | PE_RW | PE_US | PE_A | addr;

	/*
	 * No need to flush the TLB since the page table entry cannot
	 * be in the TLB (we only swap in pages after page faults).
	 */
}

/*
 * page_swap_out()
 *
 * Actually just writes the bloody page back to the
 * process image! There is no separate swap space on the USB.
 *
 * Only writes out data pages though. The text pages should
 * not have been modified, so those should just be
 * discarded.
 */
static void page_swap_out(int pageno) {
	page_map_entry_t *page = &page_map[pageno];

	scrprintf(24, 50, "pid %-3d wting page %-3d", current_running->pid, pageno);

	ASSERT((page->vaddr & PAGE_DIRECTORY_MASK) >= PROCESS_START);

	scrprintf(24, 30, "%08x", *page->entry);

	/* mark page as not present */
	*page->entry &= ~PE_P;

	/* Flush TLB */
	invalidate_page((uint32_t *)page->vaddr);

	scrprintf(24, 40, "%08x", *page->entry);

	scrprintf(24, 71, "0");

	/* if page is dirty */
	if ((*page->entry & PE_D) != 0) {
		uint32_t sector, nsectors, addr;

		sector = page_disk_sector(page);
		addr = (uint32_t)page_addr(pageno);

		/* if the final sector is past the end of the image... */
		if ((sector + SECTORS_PER_PAGE) > (page->swap_loc + page->swap_size)) {
			/* write only the sectors that belong to this image */
			nsectors = page->swap_size % SECTORS_PER_PAGE;
		}
		else {
			/* else, write an entire page */
			nsectors = SECTORS_PER_PAGE;
		}

		scsi_write(sector, nsectors, (char *)addr);
	}
	scrprintf(24, 71, "x");
}

/* Get the sector number on disk of a process image  */
static uint32_t page_disk_sector(page_map_entry_t *page) {
	return page->swap_loc + ((page->vaddr - PROCESS_START) / PAGE_SIZE) * SECTORS_PER_PAGE;
}
