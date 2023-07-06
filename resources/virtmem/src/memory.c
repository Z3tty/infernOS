/*
 * memory.c
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
#include "tlb.h"
#include "usb/scsi.h"
#include "util.h"

/* Use virtual address to get index in page directory.  */
inline uint32_t get_directory_index(uint32_t vaddr);

/*
 * Use virtual address to get index in a page table.  The bits are
 * masked, so we essentially get a modulo 1024 index.  The selection
 * of which page table to index into is done with
 * get_directory_index().
 */
inline uint32_t get_table_index(uint32_t vaddr);


/* Debug-function. 
 * Write all memory addresses and values by with 4 byte increment to output-file.
 * Output-file name is specified in bochsrc-file by line:
 * com1: enabled=1, mode=file, dev=serial.out
 * where 'dev=' is output-file. 
 * Output-file can be changed, but will by default be in 'serial.out'.
 * 
 * Arguments
 * title:		prefix for memory-dump
 * start:		memory address
 * end:			memory address
 * inclzero:	binary; skip address and values where values are zero
 */
static void 
rsprintf_memory(char *title, uint32_t start, uint32_t end, uint32_t inclzero){
	uint32_t numpage, paddr;
	char *header;

	rsprintf("%s\n", title);

	numpage = 0;
	header = "========================== PAGE NUMBER %02d ==========================\n";

	for(paddr = start; paddr < end; paddr += sizeof(uint32_t)) {

		/* Print header if address is page-aligned. */
		if(paddr % PAGE_SIZE == 0) {
			rsprintf(header, numpage);
			numpage++;
		}
		/* Avoid printing address entries with no value. */
		if(	!inclzero && *(uint32_t*)paddr == 0x0) {
			continue;
		}
		/* Print: 
		 * Entry-number from current page. 
		 * Physical main memory address. 
		 * Value at address.
		 */
		rsprintf("%04d - Memory Loc: 0x%08x ~~~~~ Mem Val: 0x%08x\n",
					((paddr - start) / sizeof(uint32_t)) % PAGE_N_ENTRIES,
					paddr,
					*(uint32_t*)paddr );
	}
}

/*
 * init_memory()
 *
 * called once by _start() in kernel.c
 * You need to set up the virtual memory map for the kernel here.
 */
void init_memory(void) {
}

/*
 * Sets up a page directory and page table for a new process or thread.
 */
void setup_page_table(pcb_t *p) {
}

/*
 * called by exception_14 in interrupt.c (the faulting address is in
 * current_running->fault_addr)
 *
 * Interrupts are on when calling this function.
 */
void page_fault_handler(void) {
}
