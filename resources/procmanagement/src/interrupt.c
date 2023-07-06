#include "common.h"
#include "interrupt.h"
#include "kernel.h"
#include "keyboard.h"
#include "memory.h"
#include "scheduler.h"
#include "util.h"

/* The following three variables are used by the dummy exception handlers. */
static uint32_t cr2; /* address that caused a page fault exception */
static uint32_t esp; /* stack pointer */
static uint32_t *s;  /* pointer to stack */

/*
 * Copy the segment descriptor index "seg" into the DS and ES
 * registers
 */
void load_data_segments(int seg) {
	asm volatile("movw %%ax, %%ds \n\t"
	             "movw %% ax, %%es " ::"a"(seg));
}

/*
 * This function handles executing a given syscall, and returns the
 * result to syscall_entry in entry.S, from where it is returned to
 * the calling process. Before we get here, syscall_entry() will have
 * stored the context of the process making the syscall, and entered
 * a critical section (through enter_critical()).
 *
 * Note:
 * The use of leave_critical() causes the interrupts to be turned on
 * again after leave_critical. (cr->disable_count goes from 1 to 0 again.)
 *
 * This makes sense if we want system calls or other interrupt
 * handlers to be interruptable (for instance allowing a timer interrupt
 * to preempt a process while it's inside the kernel in a system call).
 *
 * It does, however, also mean that we can get interrupts while we are
 * inside another interrupt handler (the same thing is done in
 * the other interrupt handlers).
 *
 * In syslib.c we put systemcall number in eax, arg1 in ebx, arg2 in ecx
 * and arg3 in edx. The return value is returned in eax.
 *
 * Before entering the processor has switched to the kernel stack
 * (PMSA p. 209, Privilege level switch whitout error code)
 */
int system_call_helper(int fn, int arg1, int arg2, int arg3) {
	int ret_val = 0;

	ASSERT2(current_running->nested_count == 0, "A process/thread that was running inside the "
	                                            "kernel made a syscall.");
	current_running->nested_count++;
	leave_critical();

	/* Call function and return result as usual (ie, "return ret_val"); */
	if (fn >= SYSCALL_COUNT || fn < 0) {
		/* Illegal system call number, call exit instead */
		fn = SYSCALL_EXIT;
	}
	/*
	 * In C's calling convention, caller is responsible for
	 * cleaning up the stack. Therefore we don't really need to
	 * distinguish between different argument numbers. Just pass all 3
	 * arguments and it will work
	 */
	ret_val = syscall[fn](arg1, arg2, arg3);

	/*
	 * We can not leave the critical section we enter here before
	 * we return in syscall_entry.
	 *
	 * This is due to a potential race condition on a scratch
	 * variable used by syscall_entry.
	 */
	enter_critical();
	current_running->nested_count--;
	ASSERT2(current_running->nested_count == 0, "Wrong nest count at B");
	return ret_val;
}

/*
 * Disable a hardware interrupt source by telling the controller to
 * ignore it.
 *
 * NOTE: each thread/ process has its own interrupt mask, so this will
 * only mask irqI for current_running. The other threads/processes can
 * still get an irq <irq> request.
 *
 * irq is interrupt number 0-7
 * UPDATE: added support for interrupts 8-15
 */
void mask_hw_int(int irq) {
	unsigned char mask;

	if (irq <= 7) {
		/* Read interrupt mask register */
		mask = inb(0x21);
		/* Disable <irq> by or'ing the mask with the corresponding bit
		 */
		mask |= (1 << irq);
		/* Write interrupt mask register */
		outb(0x21, mask);
	}
	else {
		mask = inb(0xa1);
		mask |= (1 << (irq - 8));
		outb(0xa1, mask);
	}
}

/* Unmask the hardware interrupt source indicated by irq */
void unmask_hw_int(int irq) {
	unsigned char mask;

	if (irq <= 7) {
		mask = inb(0x21);
		mask &= ~(1 << irq);
		outb(0x21, mask);
	}
	else {
		mask = inb(0xa1);
		mask &= ~(1 << (irq - 8));
		outb(0xa1, mask);
	}
}

/* See Page 1007, Chapter 17, Warnings section in The Undocumented PC */
void fake_irq7(void) {
#ifdef DEBUG
	static int fake_irq7_count = 0;
	static int iis_flag;

	/* Read Interrupt In-Service Register */
	outb(0x20, 0x0b);
	iis_flag = inb(0x20);

	/* ASSERT2(IRQ 7 not in-service) */
	ASSERT2((iis_flag & (1 << 7)) == 0, "Real irq7 !");
	scrprintf(2, 0, "Fake irq7 count: %d", ++fake_irq7_count);
#endif
}

/*
 * Exception handlers, currently they are all dummies.  The following
 * macro invocations are expanded into function definitions by the C
 * preprocessor.  Refer to PMSA p. 192 for exception categories
 */

/* Default interrupt handler */
INTERRUPT_HANDLER(bogus_interrupt, "In bogus_interrupt", FALSE);

/* Various exception handlers */
INTERRUPT_HANDLER(exception_0, "Excp. 0 - Divide by zero", FALSE);
INTERRUPT_HANDLER(exception_1, "Excp. 1 - Debug", FALSE);
INTERRUPT_HANDLER(exception_2, "Excp. 2 - NMI", FALSE);
INTERRUPT_HANDLER(exception_3, "Excp. 3 - Breakpoint", FALSE);
INTERRUPT_HANDLER(exception_4, "Excp. 4 - INTO instruction", FALSE);
INTERRUPT_HANDLER(exception_5, "Excp. 5 - BOUNDS instruction", FALSE);
INTERRUPT_HANDLER(exception_6, "Excp. 6 - Invalid opcode", FALSE);
INTERRUPT_HANDLER(exception_7, "Excp. 7 - Device not available", FALSE);
INTERRUPT_HANDLER(exception_8, "Excp. 8 - Double fault encountered", TRUE);
INTERRUPT_HANDLER(exception_9, "Excp. 9 - Coprocessor segment overrun", FALSE);
INTERRUPT_HANDLER(exception_10, "Excp. 10 - Invalid TSS Fault", TRUE);
INTERRUPT_HANDLER(exception_11, "Excp. 11 - Segment not Present", TRUE);
INTERRUPT_HANDLER(exception_12, "Excp. 12 - Stack exception", TRUE);
INTERRUPT_HANDLER(exception_13, "Excp. 13 - General Protection", TRUE);
INTERRUPT_HANDLER(exception_14, "Excp. 14 - Page fault", TRUE);
