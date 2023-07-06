#include "common.h"
#include "interrupt.h"
#include "kernel.h"
#include "process.h"
#include "scheduler.h"
#include "th.h"
#include "time.h"
#include "util.h"

/* Various static prototypes */
static void create_gate(struct gate_t *entry, uint32_t offset, uint16_t selector, char type, char privilege);
static void init_syscall(int i, syscall_t call);
static void init_pcb(void);
static void init_idt(void);

/* System call table */
syscall_t syscall[SYSCALL_COUNT];

/* Statically allocate some storage for the pcb's */
pcb_t pcb[PCB_TABLE_SIZE];
/* Ready queue and pointer to currently running process */
pcb_t *current_running;

/* Statically allocate the IDT */
static struct gate_t idt[IDT_SIZE];

/* Type of each start function-address for threads and processes. */
typedef void (*startfunc_t)(void);

/*
 * A list of start addresses for threads and processes that should be
 * started by the kernel.
 * 
 * Uncomment these functions in order to not run these.
 */
static startfunc_t start_thrds[] = {
	(startfunc_t) clock_thread,
	(startfunc_t) thread2,
	(startfunc_t) thread3,
	(startfunc_t) num,
	(startfunc_t) caps,
	(startfunc_t) scroll_th,
	(startfunc_t) mcpi_thread0,
	(startfunc_t) mcpi_thread1,
	(startfunc_t) mcpi_thread2,
	(startfunc_t) mcpi_thread3,
	
	(startfunc_t) barrier1,
	(startfunc_t) barrier2,
	(startfunc_t) barrier3
};
static startfunc_t start_procs[] = {
	(startfunc_t) PROC1_ADDR,	/* given in Makefile */
	(startfunc_t) PROC2_ADDR
};

/* Number of threads and processes to configure at start-up. 
 * Dynamically set these at start-up
 */
int numthrds, numprocs, numtotal;

/*
 * Array of addresses for the exception handlers we defined in
 * interrupt.c. Used for initializing the interrupt descriptor table.
 */
static handler_t exception_handler[NUM_EXCEPTIONS] = {exception_0, exception_1, exception_2, exception_3, exception_4, exception_5, exception_6, exception_7,
													exception_8, exception_9, exception_10, exception_11, exception_12, exception_13, exception_14};

/*
 * dispatch() requires disable_count to be 1. When we dispatch the
 * first job dispatch() sets disable_count = 0 (it requires that
 * disable_count != 0)
 */
int disable_count = 1;
int preempt_count = 0; /* Total number of preempts.. */
int yield_count = 0;   /* ..and yields */

/*
 * This is the entry point for the kernel.  Initialize processes and
 * threads, init idt, enable timer interrupt
 */
void _start(void) {
	clear_screen(0, 0, 80, 25);

	/* Tell compiler to ignore warning from casting function to arbitrary 
	 * argument-numbers while funciton takes a given set of arguments. */
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-function-type"
	/* Initialize the syscall array/table */
	init_syscall(SYSCALL_YIELD, (syscall_t)yield);
	init_syscall(SYSCALL_EXIT, (syscall_t)exit);
	init_syscall(SYSCALL_GETPID, (syscall_t)getpid);
	init_syscall(SYSCALL_GETPRIORITY, (syscall_t)getpriority);
	init_syscall(SYSCALL_SETPRIORITY, (syscall_t)setpriority);
	init_syscall(SYSCALL_CPUSPEED, (syscall_t)cpuspeed);
	/* End ignoring of function-casting warnings. */
	#pragma GCC diagnostic pop

	init_pcb();

	init_idt();

	time_init();

	/* Initialize random number generator */
	srand(214343); /* using a random value */

	/*
	 * Enable the timer interrupt.  The interrupt flag will be set
	 * once we start the first process, thus starting scheduling.
	 * Refer to the IF flag in the EFLAGS register.
	 */
	outb(0x21, 0xfe);

	/* start first job */
	dispatch();

	ASSERT(0); /* not reached */
}

static void init_pcb() {
	int i, next_stack = STACK_MIN;

	numthrds = sizeof(start_thrds) / sizeof(startfunc_t),
	numprocs = sizeof(start_procs) / sizeof(startfunc_t),
	numtotal = numthrds + numprocs;

	/* Initialize threads and processes */
	for (i = 0; i < numtotal; i++) {
		pcb[i].pid = i;
		pcb[i].is_thread = (i < numthrds) ? TRUE : FALSE;
		pcb[i].nested_count = (i < numthrds) ? TRUE : FALSE;

		/* setup kernel stack */
		ASSERT(next_stack < STACK_MAX);
		pcb[i].kernel_stack = next_stack + STACK_OFFSET;
		next_stack += STACK_SIZE;

		/* if thread; avoid user-stack allocation and set start address */
		if (pcb[i].is_thread) {
			pcb[i].start_pc = (uint32_t) start_thrds[i];
		} /* if user-process; then setup user stack and user start address */
		else {
			ASSERT(next_stack < STACK_MAX);
			pcb[i].user_stack = next_stack + STACK_OFFSET;
			next_stack += STACK_SIZE;
			
			/* e.g. if 11 threads; then i >= 11 and 
			 * processes is e.g. 12 - 11 = 1 index in process array */
			pcb[i].start_pc = (uint32_t) start_procs[i - numthrds];
		}
		pcb[i].priority = 10;
		pcb[i].status = STATUS_FIRST_TIME;

		/* insert in ready queue */
		pcb[i].next = &pcb[(i + 1) % numtotal];
		pcb[i].previous = &pcb[(i - 1 + numtotal) % numtotal];
	}
	current_running = &pcb[0];
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

/*
 * Initialize the Interrupt Descriptor Table
 *
 * IDT can contain up to 256 entries. Location (in memory) and size
 * are stored in the IDTR. Each entry conatins a descriptor. We only use
 * interrupt gate descriptors in this project. For details: PMSA chapter 12.
 *
 * In this project all processes and threads run in the same segment, so
 * there is no segment switch when a process is interrupted. Every thing runs
 * in kernel mode, so there is no stack switch when an interrupt occurs.
 *
 * When either a hardware or software interrupt occurs:
 * 1.  The processor reads the interrupt vector supplied either by the
 *     interrupt controller or instruction operand.
 * 2.  The processor multiplies the vector by eight to create the offset into
 *     the IDT.
 * 3.  The processor reads the eight byte descriptor from the IDT entry.
 * 4.  CS, EIP and EFlags are pushed on the stack.
 * 5.  EFlags[IF] is cleared to disable hardware interrupts.
 * 6.  The processor fetches the first instruction of the interrupt handler
 *     (CS:EIP).
 * 7.  The body of the interrupt routine is executed.
 * 8.  If (harware interrupt) then EOI.
 * 9.  IRET at end of the routine. This causes the processor to pop EFlags,
 *      EIP and CS
 * 10. The interrupted program resumes at the point of interruption.
 */
void init_idt(void) {
	int i;
	struct point_t idt_p;

	/* IRQs 0-15 are accosiated with IDT entrys 0-15, but so are also
	 *  some software exceptions. So we remap irq 0-15 to IDT entrys 32-48
	 * (PSMA p.187, and Und. PC p.1009) */

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

	/*
	 * Timer 0 is fed from a fixed frequency 1.1932 MHz clock,
	 * regardles of the CPU system speed (The Undocumentet PC
	 * p. 960/978)
	 */

	/* set timer 0 frequency */
	outb(0x40, (unsigned char)PREEMPT_TICKS);
	outb(0x40, PREEMPT_TICKS >> 8);

	/* create default handlers for interrupts/exceptions */
	for (i = 0; i < IDT_SIZE; i++) {
		create_gate(&(idt[i]),                 /* IDT entry */
		            (uint32_t)bogus_interrupt, /* interrupt handler */
		            KERNEL_CS,                 /* interrupt handler segment */
		            INTERRUPT_GATE,            /* gate type */
		            0);                        /* privilege level 0 */
	}

	/* Create handlers for some exceptions */
	for (i = 0; i < NUM_EXCEPTIONS; i++) {
		create_gate(&(idt[i]),                                 /* IDT entry */
		            (uint32_t)exception_handler[i], KERNEL_CS, /* exception handler segment */
		            INTERRUPT_GATE,                            /* gate type */
		            0);                                        /* privilegie level 0 */
	}

	/*
	 * create gate for the fake interrupt generated on IRQ line 7 when
	 * the timer is working at a high frequency
	 */
	create_gate(&(idt[IRQ_START + 7]), (uint32_t)fake_irq7_entry, KERNEL_CS, INTERRUPT_GATE, 0);

	/* create gate for the timer interrupt */
	create_gate(&(idt[IRQ_START]), (uint32_t)irq0_entry, KERNEL_CS, INTERRUPT_GATE, 0);

	/* create gate for system calls */
	create_gate(&(idt[IDT_SYSCALL_POS]), (uint32_t)syscall_entry, KERNEL_CS, INTERRUPT_GATE, 0);

	/* load the idtr with a pointer to our idt */
	idt_p.limit = (IDT_SIZE * 8) - 1;
	idt_p.base = (uint32_t)idt;

	/* load idtr */
	asm volatile("lidt %0" : : "m"(idt_p));
}

/*
 * General function to make a gate entry.  Refer to chapter 12, page
 * 203, in PMSA for a description of interrupt gates.
 */
void create_gate(struct gate_t *entry, /* pointer to IDT entry */
                 uint32_t offset,      /* pointer to interrupt handler */
                 uint16_t selector,    /* code segment containing the
                                        * interrupt handler */
                 /* type: interrupt gate, trap gate or task gate */
                 char type, char privilege) /* privilege level */
{
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

/* Used for debugging */
void print_status(void) {
	static char *status[] = {"First  ", "Running", "Blocked", "Exited "};
	int i, base;

	base = 20;
	print_str(base - 4, 5, "P R O C E S S    S T A T U S");
	print_str(base - 2, 0, "Pid");
	print_str(base - 2, 10, "Type");
	print_str(base - 2, 20, "Priority");
	print_str(base - 2, 30, "Status");
	for (i = 0; i < numtotal && (base + i) < 25; i++) {
		print_int(base + i, 0, pcb[i].pid);
		print_str(base + i, 10, pcb[i].is_thread ? "Thread" : "Process");
		print_int(base + i, 20, pcb[i].priority);
		print_str(base + i, 30, status[pcb[i].status]);
	}
}
