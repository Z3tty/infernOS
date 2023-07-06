#include "common.h"
#include "dispatch.h"
#include "interrupt.h"
#include "kernel.h"
#include "keyboard.h"
#include "mbox.h"
#include "memory.h"
#include "paging.h"
#include "scheduler.h"
#include "screen.h"
#include "th.h"
#include "time.h"
#include "usb/scsi.h"
#include "usb/usb.h"
#include "util.h"

/* Various static prototypes */
static inline void enable_paging(void);

static void create_gate(struct gate_t *entry, uint32_t offset, uint16_t selector, char type, char privilege);
static void create_segment(struct segment_t *entry, uint32_t base, uint32_t limit, char type, char privilege, char seg_type);
static void init_syscall(int i, syscall_t call);
static void init_idt(void);
static void init_gdt(void);
static void init_tss(void);
static void init_pcb_table(void);
static int create_thread(int i);
static int create_process(uint32_t location, uint32_t size);
static pcb_t *alloc_pcb();

/* System call table */
syscall_t syscall[SYSCALL_COUNT];

/* Statically allocate some storage for the pcb's */
pcb_t pcb[PCB_TABLE_SIZE];
/* Ready queue and pointer to currently running process */
pcb_t *current_running;

/* Statically allocate the IDT,  GDT and TSS */
static struct gate_t idt[IDT_SIZE];
static struct segment_t gdt[GDT_SIZE];
struct tss_t tss;

/* Used for allocation of pids, kernel stack, and pcbs */
static pcb_t *next_free_pcb;
static int next_pid = 0;
static int next_stack = STACK_MIN;

/*
 * A list of start addresses for threads that should be started by the
 * kernel. 
 * Uncomment these functions to disable them starting up.
 */
static func_t start_addr[] = {
    (func_t)loader_thread, /* Loads shell */
    (func_t)clock_thread,  /* Running indefinitely */
    (func_t)usb_thread,    /* Scans USB hub port */
    (func_t)thread2,       /* Test thread */
    (func_t)thread3        /* Test thread */
};

/*
 * Array of addresses for the exception handlers we defined in
 * interrupt.c. Used for initializing the interrupt descriptor table.
 */
static handler_t exception_handler[NUM_EXCEPTIONS] = {exception_0, exception_1, exception_2, exception_3, exception_4, exception_5, exception_6, exception_7, exception_8, exception_9, exception_10, exception_11, exception_12, exception_13, exception_14};

/* This is the entry point for the kernel */
FUNCTION_ALIAS(kernel_start, _start);
void _start(void) {
	int i, numthreads;

	CLI(); /* just in case the interrupts are not disabled */

	clear_screen(0, 0, 80, 25);

	/* Disables intentional warning about e.g. 
	 * setpriority() being different function-type than (func_t) */
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-function-type"
	/* Initialize the syscall array/table */
	init_syscall(SYSCALL_YIELD, (syscall_t)yield);
	init_syscall(SYSCALL_EXIT, (syscall_t)exit);
	init_syscall(SYSCALL_GETPID, (syscall_t)getpid);
	init_syscall(SYSCALL_GETPRIORITY, (syscall_t)getpriority);
	init_syscall(SYSCALL_SETPRIORITY, (syscall_t)setpriority);
	init_syscall(SYSCALL_CPUSPEED, (syscall_t)cpuspeed);
	init_syscall(SYSCALL_MBOX_OPEN, (syscall_t)mbox_open);
	init_syscall(SYSCALL_MBOX_CLOSE, (syscall_t)mbox_close);
	init_syscall(SYSCALL_MBOX_STAT, (syscall_t)mbox_stat);
	init_syscall(SYSCALL_MBOX_RECV, (syscall_t)mbox_recv);
	init_syscall(SYSCALL_MBOX_SEND, (syscall_t)mbox_send);
	init_syscall(SYSCALL_GETCHAR, (syscall_t)getchar);
	init_syscall(SYSCALL_READDIR, (syscall_t)readdir);
	init_syscall(SYSCALL_LOADPROC, (syscall_t)loadproc);
	#pragma GCC diagnostic pop

	/* Initialize IDT, GDT, TSS and the PCB table */
	init_idt();
	init_gdt();
	init_tss();
	init_pcb_table();

	/* Initialize various "subsystems" */
	init_memory();
	mbox_init();
	time_init();
	keyboard_init();
	scsi_static_init();
	usb_static_init();

	/*
	 * Set up the page directory for the kernel and kernel
	 * threads.  Create kernel page table, and identity map the
	 * first 0-640KB, Video RAM and MEM_START-MEM_END, with kernel
	 * access privileges. */
	make_kernel_page_directory();

	/* Create the threads */
	numthreads = sizeof(start_addr) / sizeof(func_t);
	for (i = 0; i < numthreads; i++) {
		create_thread(i);
	}

	/*
	 * Select the page directory of the first thread in the ready
	 * queue. Then enable paging before dispatching
	 */
	select_page_directory();
	enable_paging();

	/* Start the first thread */
	dispatch();

	/* NOT REACHED */
	ASSERT(0);
}

/*
 * Loads the pointer to current_running's page directory into CR3,
 * which means that we start using the memory map of that process
 */
void select_page_directory(void) {
	asm volatile("movl %%eax, %%cr3 " ::"a"(current_running->page_directory));
}

/*
 * General function to make a gate entry. Refer to chapter 12, page
 * 203, in PMSA for a description of interrupt gates.
 */
void create_gate(struct gate_t *entry, /* pointer to IDT entry */
                 uint32_t offset,      /* pointer to interrupt handler */
                 uint16_t selector,    /* code segment containing the
                                        * interrupt handler */
                 /* type (interrupt gate, trap gate or task gate) */
                 char type, char privilege /* privilege level */) {
	entry->offset_low = (uint16_t)offset;
	entry->selector = (uint16_t)selector;
	/* Byte 4 [0:4] =  Reserved
	 * Byte 4 [5:7] =  0,0,0 */
	entry->count = 0;
	/* Byte 5 [0:2] =  type[0:2]
	 * Byte 5 [3]   =  type[3] (1: 32-bit, 0: 16-bit)
	 * Byte 5 [4]   =  0 (indicates system segment)
	 * Byte 5 [5:6] =  privilege
	 * Byte 5 [7]   =  1 (segment always present) */
	entry->access = type | privilege << 5 | 1 << 7;
	entry->offset_high = (uint16_t)(offset >> 16);
}

/* General function to make a segment descriptor. Refer to chapter 7,
 * PMSA p.95 for a description of segment descriptors.
 */
void create_segment(struct segment_t *entry, /* GDT entry */
                    uint32_t base,           /* segment start address */
                    uint32_t limit,          /* segment size */
                    char type,               /* segment type (OS system-,
                              code-, data-, stack segment) */
                    char privilege,          /* descriptor privilege level */
                    char system /* system bit (0: system segm.) */) {
	entry->limit_low = (uint16_t)limit;
	entry->limit_high = (uint8_t)(limit >> 16);
	entry->base_low = (uint16_t)base;
	entry->base_mid = (uint8_t)(base >> 16);
	entry->base_high = (uint8_t)(base >> 24);
	/* Byte 5 [0:3] = type
	 * Byte 5 [4]   = system
	 * Byte 5 [5:6] = privilege level
	 * Byte 5 [7]   = 1 (always present) */
	entry->access = type | system << 4 | privilege << 5 | 1 << 7;
	/* Byte 6 [0:3] = limit_high
	 * Byte 6 [4]   = 0 (not avilable for use by system software)
	 * Byte 6 [5]   = 0 (reserved)
	 * Byte 6 [6]   = 1 (D/B bit)
	 * Byte 6 [7]   = 1 (length of limit is in pages) */
	entry->limit_high |= 1 << 7 | 1 << 6;
}

/*
 * Called by kernel to assign a system call handler to the array of
 * system calls.
 */
static void init_syscall(int i, syscall_t call) {
	ASSERT((i >= 0) && (i < SYSCALL_COUNT));
	ASSERT(call != NULL);
	syscall[i] = call;
}

/* This function enables paging by setting CR0[31] to 1. */
static inline void enable_paging() {
	__asm__ volatile("movl  %cr0,%eax    \n\t"
	                 "orl  $0x80000000,%eax  \n\t"
	                 "movl  %eax,%cr0    \n\t");
}

/* Initialize the Interrupt Descriptor Table
 *
 * IDT can contain up to 256 entries. Location (in memory) and size
 * are stored in the IDTR. Each entry conatins a descriptor. We only use
 * interrupt gate descriptors in this project. For details: PMSA chapter 12.
 *
 * When either a hardware or software interrupt occurs:
 * 1.  The processor reads the interrupt vector supplied either by the
 *     interrupt controller or instruction operand.
 * 2.  The processor multiplies the vector by eight to create the offset into
 *     the IDT.
 * 3.  The processor reads the eight byte descriptor from the IDT entry.
 * 4.  The processor checks that IDT[entry].DPL <= CS.DPL
 * 5.  Read GDT[ IDT[entry].selector ], to obtain the code segment
 *     start address and size.

 * 6.  Processor checks that IDT[entry].CS.DPL <= CS.CPL
 * 7.  If (IDT[entry].CS.DPL <= CS.CPL) then the processor switchs to the
 *     kernel stack by:
 * 7a. Saving SS and ESP temporarly.
 * 7b. Creating a new stack (the kernel stack) by loading tss.ss and tss.esp.
 * 7c. Pushing temporarly saved SS and ESP on the kernel stack.
 * 8.  CS, EIP and EFlags are pushed on the stack.
 * 9.  EFlags[IF] is cleared to disable hardware interrupts.
 * 10. IDT[entry].selector is loaded into CS (and the CS.cache is loaded from
 *     the GDT)
 *     IDT[entry].offset is moved into EIP
 * 11. The processor fetches the first instruction of the interrupt handler
 *     (CS:EIP).
 * 12. The body of the interrupt routine is executed.
 * 13. If (hardware interrupt) then EOI.
 * 14. IRET at end of the routine. This causes the processor to pop EFlags,
 *      EIP and CS
 * 14. If (switched to kernel stack) then user stack is restored by poping
 *      ESP and SS from the kernel stack.
 * 15. The interrupted program resumes at the point of interruption.
 */
static void init_idt(void) {
	int i;
	struct point_t idt_p;

	/*
	 * IRQs 0-15 are associated with IDT entries 0-15, but so are
	 * also some software exceptions. So we remap irq 0-15 to IDT
	 * entrys 32-48 (PSMA p.187, and Und. PC p.1009)
	 */

	/* interrupt controller 1 */
	outb(0x20, 0x11);      /* Start init of controller 0, require 4 bytes */
	outb(0x21, IRQ_START); /* IRQ 0-7 use vectors 0x20-0x27 (32-39)  */
	outb(0x21, 0x04);      /* Slave controller on IRQ 2 */
	outb(0x21, 0x01);      /* Normal EOI nonbuffered, 80x86 mode */
	outb(0x21, 0xfb);      /* Disable int 0-7, enable int 2 */

	/* interrupt controller 2 */
	outb(0xa0, 0x11);          /* Start init of controller 1, require 4 bytes */
	outb(0xa1, IRQ_START + 8); /* IRQ 8-15 use vectors 0x28-0x30 (40-48) */
	outb(0xa1, 0x02);          /* Slave controller id, slave on IRQ 2 */
	outb(0xa1, 0x01);          /* Normal EOI nonbuffered, 80x86 mode */
	outb(0xa1, 0xff);          /* Disable int 8-15 */

	/* Set timer frequency */
	reset_timer();

	/* Create default handlers for interrupts/exceptions */
	for (i = 0; i < IDT_SIZE; i++) {
		create_gate(&(idt[i]), (uint32_t)bogus_interrupt, KERNEL_CS, INTERRUPT_GATE, 0);
	}

	/* Create handlers for some exceptions */
	for (i = 0; i < NUM_EXCEPTIONS; i++) {
		create_gate(&(idt[i]), /* IDT entry */
		            /* exception handler segment */
		            (uint32_t)exception_handler[i], KERNEL_CS, INTERRUPT_GATE, /* interrupt gate type */
		            0);                                                        /* privilege level */
	}

	/*
	 * create gate for the fake interrupt generated on IRQ line 7
	 * when the timer is working at a high frequency
	 */
	create_gate(&(idt[IRQ_START + 7]), (uint32_t)fake_irq7_entry, KERNEL_CS, INTERRUPT_GATE, 0);

	/* Create gate for the timer interrupt */
	create_gate(&(idt[IRQ_START]), (uint32_t)irq0_entry, KERNEL_CS, INTERRUPT_GATE, 0);

	/* Create gate for the keyboard interrupt */
	create_gate(&(idt[IRQ_START + 1]), (uint32_t)irq1_entry, KERNEL_CS, INTERRUPT_GATE, 0);

	/* Create gates for the PCI interrupts */
	create_gate(&(idt[IRQ_START + 5]), (uint32_t)pci5_entry, KERNEL_CS, INTERRUPT_GATE, 0);
	create_gate(&(idt[IRQ_START + 9]), (uint32_t)pci9_entry, KERNEL_CS, INTERRUPT_GATE, 0);
	create_gate(&(idt[IRQ_START + 10]), (uint32_t)pci10_entry, KERNEL_CS, INTERRUPT_GATE, 0);
	create_gate(&(idt[IRQ_START + 11]), (uint32_t)pci11_entry, KERNEL_CS, INTERRUPT_GATE, 0);

	/* Create gate for system calls */
	create_gate(&(idt[IDT_SYSCALL_POS]), (uint32_t)syscall_entry, KERNEL_CS, INTERRUPT_GATE, 3);

	/* Load the idtr with a pointer to our idt */
	idt_p.limit = (IDT_SIZE * 8) - 1;
	idt_p.base = (uint32_t)idt;

	asm("lidt %0" : : "m"(idt_p));
}

/*
 * Replaces the GDT that was set up in the boot block
 *
 * GDT can contain 8192 entrys. Entry zero is not used. There can be
 *  5 different descriptors in the GDT:
 *  - One TSS for each task (we have only one for all tasks)
 *  - One or more Local Descriptor Table (LDT) descriptors (not used)
 *  - Descriptors for shared code or data/stack segments for memory that
 *    may be accesed by multiple tasks
 *  - Procedure Call Gates (not used)
 *  - Task gates (not used)
 *
 * We have 5 entries in our GDT: kernel code segment, kernel data segment
 * (both with the highest privilege level), process code segment,
 * process code segment (both with lowest privilege level), and one
 * TSS segment.
 *
 * GDT is used when the processor attempts to access a new memory
 * segment, i.e. loads a descriptor into a segment register (far jump,
 * far call, hardware interrupt, software exception, software interrupt,
 * far return, iret, or "movl %eyx, %xs").
 *
 * Bits [15..3] in the segment register is used to index the GDT.
 *
 * Since each GDT entry is 8 bytes, we can calculate the memory
 * location by adding the index * 8 to the GDT base. The processor then loads
 * the segment descriptor from the GDT to the invisible part of the segment
 * register.
 *
 * To summarize: The GDT is accessed when we switch cs, or we manually switch
 * to another data segment. We do this when go from user mode to kernel mode
 * and from kernel mode to user mode.
 *
 * For more information about GDT, code segments and data segments refer to
 *  PMSA Chapters 7, 8, and 9
 */
void init_gdt(void) {
	struct point_t gdt_p;

	/* Create selectors for kernel, user space and the tss */

	/* Code segment for the kernel */
	create_segment(gdt + KERNEL_CODE, /* gdt entry */
	               0,                 /* memory start address */
	               0xfffff,           /* size (4 GB) */
	               CODE_SEGMENT,      /* type = code segment */
	               0,                 /* highest privilege level */
	               MEMORY);           /* is not a system segment */

	/* Data segment for the kernel (contains kernel stack) */
	create_segment(gdt + KERNEL_DATA, 0, 0xfffff, DATA_SEGMENT, 0, /* highest privilege level */
	               MEMORY);

	/* Code segment for processes */
	create_segment(gdt + PROCESS_CODE, 0, 0xfffff, CODE_SEGMENT, 3, /* lowest privilege level */
	               MEMORY);

	/* Data segment for processes (contains user stack) */
	create_segment(gdt + PROCESS_DATA, 0, 0xfffff, DATA_SEGMENT, 3, /* lowest privilege level */
	               MEMORY);

	/* Insert pointer to the global TSS */
	create_segment(gdt + TSS_INDEX, (uint32_t)&tss, TSS_SIZE, TSS_SEGMENT, 0, /* highest privilege level */
	               SYSTEM);                                                   /* is a system segment */

	/*
	 * Load the GDTR register with a pointer to the gdt, and the
	 * size of the gdt
	 */
	gdt_p.limit = GDT_SIZE * 8 - 1;
	gdt_p.base = (uint32_t)gdt;

	asm volatile("lgdt %0" ::"m"(gdt_p));

	/* Reload the Segment registers to refresh the hidden portions */
	asm volatile("pushl %ds");
	asm volatile("popl %ds");

	asm volatile("pushl %es");
	asm volatile("popl %es");

	asm volatile("pushl %ss");
	asm volatile("popl %ss");
}

/* Initialize task state segment. This OS only uses one task state
 * segment, so the backlink points back at itself. We don't use the
 * processors multitasking mechanism.
 *
 * We use the tss to switch from the user stack to the kernel stack.
 * This happens only when a process running in privilege level 3  is
 * interrupted and enters the kernel (which has privilege level 0).
 * See the description init_idt().
 *
 * The pointer to the kernel stack (tss.ss0:tss.esp0) is set every time a
 * process is dispatched.
 */
void init_tss(void) {
	uint16_t tss_p = KERNEL_TSS;

	tss.esp_0 = 0;             /* Set in setup_current_running() in */
	tss.ss_0 = 0;              /*  scheduler.c */
	tss.ldt_selector = 0;      /* No LDT, all process use same segments */
	tss.backlink = KERNEL_TSS; /* Use the same TSS on interrupts */
	/*
	 * 16-bit offset from the base of the TSS to the I/O
	 * permission bitmap and interrupt redirection bitmap (which
	 * is not used)
	 */
	tss.iomap_base = sizeof(struct tss_t);

	/* The rest of the fields are not used */

	/* Load the index in the GDT corresponding to the TSS
	 * into the Task Register (p 153 PMSA).
	 */
	asm volatile("ltr %0" ::"m"(tss_p));
}

/* Initialize pcb table before allocating pcbs */
static void init_pcb_table() {
	int i;

	/* link all the pcbs together in the next_free_pcb list */
	for (i = 0; i < PCB_TABLE_SIZE - 1; i++)
		pcb[i].next = &pcb[i + 1];

	pcb[PCB_TABLE_SIZE - 1].next = NULL;

	next_free_pcb = pcb;
	/* current_running also serves as the ready queue pointer */
	current_running = NULL;
}

/*
 * Allocate and set up the pcb for a new thread, allocate resources
 * for it and insert it into the ready queue.
 *
 * Called by _start() before interrupts are enabled.
 */
int create_thread(int i) {
	pcb_t *p = alloc_pcb();

	p->pid = next_pid++;
	p->is_thread = TRUE;

	ASSERT2(next_stack < STACK_MAX, "Out of stack space");
	p->kernel_stack = p->base_kernel_stack = next_stack + STACK_OFFSET;
	next_stack += STACK_SIZE;

	p->base = 0;  /* only set for processes */
	p->limit = 0; /* only set for processes */
	p->first_time = TRUE;
	p->priority = 10;
	p->status = RUNNING;
	p->nested_count = 0;
	p->disable_count = 1;
	p->preempt_count = 0;
	p->yield_count = 0;
	/* Enable keyboard, timer, fake_irq7, and PCI interrupts */
	p->int_controller_mask = 0xf1d8;

	p->user_stack = 0; /* threads don't have a user stack */
	                   /*
	                    * All threads runs in kernel address space, the start address
	                    * is known.
	                    */
	p->start_pc = (uint32_t)start_addr[i];
	p->cs = KERNEL_CS; /* Kernel code segment selector, RPL = 0 */
	p->ds = KERNEL_DS;

	/*
	 * Allocate and set up the page directory and any necessary
	 * page tables.  It only sets p->page_directory =
	 * kernel_page_directory.
	 */
	make_page_directory(p);

	return 0;
}

/* Allocate and set up the pcb for a new process, allocate resources
 * for it and insert it into the ready queue.
 *
 * CRITICAL_SECTION is used since we touch and modify global
 * state in this function (interrupts are enabled in syscall entry).
 * Note that the pcb is inserted in the ready queue in alloc_pcb, so
 * we cannot turn on the interrupts before we are done creating the process
 *
 * 'base' is base physical memory, and 'limit' is size in bytes
 */
int create_process(uint32_t base, uint32_t size) {
	pcb_t *p;

	enter_critical();
	/* Allocate pcb and insert in ready queue */
	p = alloc_pcb();

	p->pid = next_pid++;
	p->is_thread = FALSE;

	/* allocate kernel stack */
	ASSERT2(next_stack < STACK_MAX, "Out of stack space");
	p->kernel_stack = p->base_kernel_stack = next_stack + STACK_OFFSET;
	next_stack += STACK_SIZE;

	p->base = base;
	p->limit = size;
	p->first_time = TRUE;
	p->priority = 10;
	p->status = RUNNING;
	p->nested_count = 0;
	p->disable_count = 1;
	p->preempt_count = 0;
	p->yield_count = 0;
	/* Enable keyboard, timer, and PCI interrupts */
	p->int_controller_mask = 0xf1d8;
	/* setup user stack */
	p->user_stack = PROCESS_START + size - STACK_SIZE + STACK_OFFSET;
	/* Each processes has its own address space */
	p->start_pc = PROCESS_START;

	/* process code segment selector, with RPL = 3 (user mode) */
	p->cs = PROCESS_CS | 3;
	p->ds = PROCESS_DS | 3;

	/*
	 * Memory is allocated for a page directory and two page tables. Two
	 * page tables are created, one for kernel memory (v.addr: 0..MEM_END)
	 * and one for process memory (v.addr: PROCESS_START..).  * 0..640KB,
	 * and MEM_START..MEM_END is identity mapped with kernel access
	 * privileges. Video RAM is identity mapped with user aceess
	 * privileges.  * PROCESS_START..(PROCESS_START + process size) is
	 * mapped into the allocated base..(base + limit), with user
	 * privileges.  * Finally p->page_directory points to the newly
	 * created page directory.
	 */
	make_page_directory(p);

	leave_critical();
	return 0;
}

/* Get a free pcb
 *
 * NOTE: Interrupts should be disabled before calling this function.
 * Only create_process() and create_thread() call this function.
 */
pcb_t *alloc_pcb() {
	pcb_t *p;

	ASSERT2(next_free_pcb, "Running out of free PCB!");

	p = next_free_pcb;
	next_free_pcb = p->next;

	/* we automatically insert this new pcb into the ready queue */
	if (current_running == NULL) {
		current_running = p;
		p->next = p;
		p->previous = p;
	}
	else {
		p->next = current_running;
		p->previous = current_running->previous;
		p->next->previous = p;
		p->previous->next = p;
	}

	return p;
}

/* Remove the pcb from its current list, and put it back into the free list.
 *
 * NOTE: must be called whitin a critical section. Only called in scheduler()
 * when stauts == EXITED.
 */
void free_pcb(struct pcb *p) {
	/* Remove the pcb from its current queue... */
	p->previous->next = p->next;
	p->next->previous = p->previous;
	/* ...and insert into the free list */
	p->next = next_free_pcb;
	next_free_pcb = p;
}

/*
 * Used for debugging. The clock thread will call this function every
 * so often to update the status info.
 */
void print_status(int time) {
	static char *status[] = {"Running ", "Blocked ", "Sleeping", "Exited  "};
	int i, j, base;
	pcb_t *p;

	base = CLOCK_BASE_LINE;

	scrprintf(base - 5, 12, "P R O C E S S    S T A T U S   after %d seconds", time);
	scrprintf(base - 3, 0, "%-5s%-10s%-10s%-10s%-10s%-10s%-10s%-10s", "Pid", "Type", "Status", "Disable", "Preempt", "Yield", "Page", "Kernel");
	scrprintf(base - 2, 0, "%-5s%-10s%-10s%-10s%-10s%-10s%-10s%-10s", "", "", "", "count", "count", "", "faults", "stack");
	enter_critical();
	p = current_running;
	i = 0;
	do {
		scrprintf(base + i, 0, "%-5d%-10s%-10s%-10d%-10d%-10d%-10d%-10x", p->pid, p->is_thread ? "Thread" : "Process", status[p->status], p->disable_count, p->preempt_count, p->yield_count, 0, p->kernel_stack);
		p = p->next;
		i++;
	} while ((p != current_running) && (i < 6));
	leave_critical();

	for (; i < 6; i++)
		for (j = 0; j < 80; j++)
			scrprintf(base + i, j, " ");
}

/*
 * Read the directory from the usb and copy it to the
 * user provided buf.
 *
 * NOTE that block size equals sector size.
 * 
 * NOTE avoid passing 'buf'-argument directly to scsi_read(),
 * use an intermidary buffer on the stack. 
 * This is not a problem when loader_thread() spawns the shell
 * since that argument address is in kernel-space. 
 * The problem comes when the virtual-address argument pointer 
 * refers to something in process-space. Which is not a problem 
 * in kernel space because addresses are one-to-one with 
 * physical-address. 
 */
int readdir(unsigned char *buf) {
	/* Only for -Werror - remove once the code is completed */
	return -1;
}

/* Load a process from disk.
 * Location is sector number on disk, size is number of sectors.
 * Interrupts are enabled but global state is not modifyed.
 */
int loadproc(int location, int size) {
	/* Only for -Werror - remove once the code is completed */
	return -1;
}

/* Reset timer 0 with the frequency specified by PREEMPT_TICKS. */
void reset_timer(void) {
	outb(0x40, (uint8_t)PREEMPT_TICKS);
	outb(0x40, (uint8_t)(PREEMPT_TICKS >> 8));
}
