#ifndef KERNEL_H
#define KERNEL_H

#include "common.h"
#include "fstypes.h"

enum
{
	KERNEL_CODE = 1,
	KERNEL_DATA,
	PROCESS_CODE,
	PROCESS_DATA,
	TSS_INDEX,

	/* Pointer to top of empty process stack */
	PROCESS_STACK = 0xEFFFFFF0,

	/*
	 * These are used for the code, data and task state segment
	 * registers. The numbers are indexes into the global descriptor
	 * table. Shiftlefted since the contents of the register is more
	 * than just the index. (PMSA p. 80).
	 */
	KERNEL_CS = KERNEL_CODE << 3,
	KERNEL_DS = KERNEL_DATA << 3,
	PROCESS_CS = PROCESS_CODE << 3,
	PROCESS_DS = PROCESS_DATA << 3,
	KERNEL_TSS = TSS_INDEX << 3,

	/* Segment descriptor types (used in create_segment) */
	CODE_SEGMENT = 0x0A,
	DATA_SEGMENT = 0x02,
	TSS_SEGMENT = 0x09,

	/* Used to set the system bit in create_segment() */
	MEMORY = 1,
	SYSTEM = 0,

	/* Used to define size of a TSS segment */
	TSS_SIZE = 103,

	/*
	 * Used to set Timer 0 frequency For more details on how to set
	 * PREEMPT_TICKS refer to: The Undocumented PC p.978-979 (Port
	 * 40h).  Alternate values are for instance 1193 (1000 Hz) and 119
	 * (10 000 Hz). The last value will be too have the timer
	 * interrupt fire too often, so try something higher!
	 */
	PREEMPT_TICKS = 11932, /* Timer interrupt at 100 Hz */

	/* Number of pcbs the OS supports */
	PCB_TABLE_SIZE = 128,

	/* kernel stack allocator constants */
	STACK_MIN = 0x40000,
	STACK_MAX = 0x80000,
	STACK_OFFSET = 0x1FFC,
	STACK_SIZE = 0x2000,

	/*
	 * IDT - Interrupt Descriptor Table
	 * GDT - Global Descriptor Table
	 * TSS - Task State Segment
	 */
	GDT_SIZE = 7,
	IDT_SIZE = 49,
	IRQ_START = 32,        /* remapped irq0 IDT entry */
	INTERRUPT_GATE = 0x0E, /* interrupt gate descriptor */
	IDT_SYSCALL_POS = 48,  /* system call IDT entry */
};

#ifndef LINUX_SIM

/*
 * The process control block is used for storing various information
 * about a thread or process
 */
struct pcb {
	uint32_t pid;        /* Process id of this process */
	uint32_t is_thread;  /* Thread or process */
	uint32_t user_stack; /* Pointer to base of the user stack */
	/*
	 * Used to set up kernel stack, and temporary save esp in
	 * scheduler()/dispatch()
	 */
	uint32_t kernel_stack;
	/*
	 * Pointer to base of the kernel stack (used to restore an
	 * empty ker. st.)
	 */
	uint32_t base_kernel_stack;
	/*
	 * When disable_count == 0, we can turn on interrupts. See
	 * enter_critical and leave_critical in entry.S
	 */
	uint32_t disable_count;
	/* Number of times process has been preempted */
	uint32_t preempt_count;
	uint32_t nested_count; /* Number of nested interrupts */
	uint32_t start_pc;     /* Start address of a process or thread */
	uint32_t ds;           /* Data segment selector */
	uint32_t cs;           /* Code segment selector */
	uint32_t fault_addr;   /* Location that generated a page fault */
	uint32_t error_code;   /* Error code associated with a page fault */
	uint32_t swap_loc;     /* Swap space base address */
	uint32_t swap_size;    /* Size of this process */
	/* True before this process has had a chance to run */
	uint32_t first_time;
	uint32_t priority;         /* This process' priority */
	uint32_t status;           /* RUNNING, BLOCKED, SLEEPING or EXITED */
	uint32_t page_fault_count; /* Number of page faults */
	uint32_t yield_count;      /* Number of yields made by this process */
	/* Interrupt controller mask (bit x = 0, enable irq x). */
	uint32_t int_controller_mask;
	/*
	 * Time at which this process should transition from SLEEPING
	 * to RUNNING.
	 */
	uint64_t wakeup_time;
	/* Used when job is in some waiting queue */
	struct pcb *next_blocked;
	uint32_t *page_directory; /* Virtual memory page directory */

	/* filesystem stuff */
	inode_t cwd;
	struct fd_entry filedes[MAX_OPEN_FILES];

	struct pcb *next;     /* Used when job is in the ready queue */
	struct pcb *previous; /* Used when job is in the ready queue */
};

#else  /* LINUX_SIM */

/*
 * This one is used to simulate a PCB.
 */
struct pcb {
	inode_t cwd;
	struct fd_entry filedes[MAX_OPEN_FILES];
};
#endif /* !LINUX_SIM */

typedef struct pcb pcb_t;

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
 * Structure describing the contents of a segment descriptor entry
 * (Protected Mode Software Architecture p.95)
 */
struct segment_t {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t limit_high;
	uint8_t base_high;
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

/*
 * Structure describing the contents of a Task State Segment
 * (Protected Mode Software architecture p.141)
 */
typedef struct tss_t {
	uint32_t backlink;
	uint32_t esp_0;
	uint16_t ss_0;
	uint16_t pad0;
	uint32_t esp_1;
	uint16_t ss_1;
	uint16_t pad1;
	uint32_t esp_2;
	uint16_t ss_2;
	uint16_t pad2;
	uint32_t reserved;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint16_t es;
	uint16_t pad3;
	uint16_t cs;
	uint16_t pad4;
	uint16_t ss;
	uint16_t pad5;
	uint16_t ds;
	uint16_t pad6;
	uint16_t fs;
	uint16_t pad7;
	uint16_t gs;
	uint16_t pad8;
	uint16_t ldt_selector;
	uint16_t pad9;
	uint16_t debug_trap;
	uint16_t iomap_base;
} __attribute((packed)) tss_t;

/* Defining a function pointer is easier when we have a type. */
/* Syscalls return an int. Don't specify arguments! */
typedef int (*syscall_t)();
/* Exception handlers don't return anything */
typedef void (*handler_t)(void);

/* Function pointer typedef */
typedef void (*func_t)(void);

/*
 * Global table with pointers to the kernel functions implementing the
 * system calls. System call number 0 has index 0 here.
 */
extern syscall_t syscall[SYSCALL_COUNT];

/* An array of pcb structures we can allocate pcbs from */
extern pcb_t pcb[];

/* The currently running process, and also a pointer to the ready queue */
extern pcb_t *current_running;

/* The global shared tss for all processes */
extern tss_t tss;

/*
 * Load pointer to the page directory of current_running into the CR3
 * register.
 */
void select_page_directory(void);
/* Print some debug info */
void print_status(int time);
/* Reset timer 0 to the frequency specified by PREEMPT_TICKS */
void reset_timer(void);

#endif /* !KERNEL_H */
