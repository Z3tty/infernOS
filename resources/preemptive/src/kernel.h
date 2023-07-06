/*
 * Various definitions used by the kernel and related code.
 */
#ifndef KERNEL_H
#define KERNEL_H

#include "common.h"
#include "pcb.h"

enum
{
	/*
	 * Used to set Timer 0 frequency For more details on how to set
	 *  PREEMPT_TICKS refer to: The Undocumented PC p.978-979 (Port
	 *  40h).  Alternate values are for instance 1193 (1000 Hz) and 119
	 *  (10 000 Hz). The last value will be too have the timer
	 *  interrupt fire too often, so try something higher!
	 */
	PREEMPT_TICKS = 11932, /* Timer interrupt at 100 Hz */
	/*
	 * Number of threads and processes initially started by the
	 * kernel. Change this when adding to or removing elements from
	 * the start_addr array.
	 */
	NUM_THREADS = 13,
	NUM_PROCS = 2,
	NUM_TOTAL = (NUM_PROCS + NUM_THREADS),

	/* Number of pcbs the OS supports */
	PCB_TABLE_SIZE = NUM_TOTAL,

	/* kernel stack allocator constants */
	STACK_MIN = 0x20000,
	STACK_MAX = 0x80000,
	STACK_OFFSET = 0x0FFC,
	STACK_SIZE = 0x1000,

	/* Kernel code segment descriptor */
	KERNEL_CS = 1 << 3,
	/*
	 * IDT - Interrupt Descriptor Table
	 * GDT - Global Descriptor Table
	 * TSS - Task State Segment
	 */
	GDT_SIZE = 7,
	IDT_SIZE = 49,
	IRQ_START = 32,        /* remapped irq0 IDT entry */
	INTERRUPT_GATE = 0x0E, /* interrupt gate descriptor */
	IDT_SYSCALL_POS = 48   /* system call IDT entry */
};

/*
 * Structure describing the contents of an interrupt gate entry.
 * (Protected Mode Software Architecture p. 206)
 */
struct gate_t {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t count;
	uint8_t access;
	uint16_t offset_high;
} __attribute__((packed));

/*
 * Structure describing the contents of the idt and gdt registers.
 * Used for loading the idt and gdt registers.  (Protected Mode
 * Software architecture p.42, )
 */
struct point_t {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

/* Defining a function pointer is easier when we have a type. */
/* Syscalls return an int. Don't specify arguments! */
typedef int (*syscall_t)();
/* Exception handlers don't return anything */
typedef void (*handler_t)(void);

/*
 * Global table with pointers to the kernel functions implementing the
 * system calls. System call number 0 has index 0 here.
 */
extern syscall_t syscall[SYSCALL_COUNT];

/* An array of pcb structures we can allocate pcbs from */
extern pcb_t pcb[NUM_TOTAL];

/* The currently running process, and also a pointer to the ready queue */
extern pcb_t *current_running;

/*
 * Used in enter/leave_critical. We increase d_c for every nested
 * critical section, and we re-enable interrupts in leave_critical if
 * d_c == 0
 */
extern int disable_count;

/* Total number of preemptions and yields */
extern int preempt_count;
extern int yield_count;

/* Print some debug info */
void print_status(void);

#endif /* !KERNEL_H */
