# entry.S

#include "asmsyms.h"
		
# The code in this file serves as an entry point for most
# interrupts. The file is
# pre-processed before it is assembled, allowing (among other things)
# the use of macros and C-style comments.

# Save all registers, except eflags
	
#define SAVE_GEN_REGS				\
	pushl %eax ;				\
	pushl %ebx ;				\
	pushl %ecx ;				\
	pushl %edx ;				\
	pushl %edi ;				\
	pushl %esi ;				\
	pushl %ebp ; 
	
# Restore all registers, except eflags
	
#define RESTORE_GEN_REGS			\
	popl %ebp ;				\
	popl %esi ;				\
	popl %edi ;				\
	popl %edx ;				\
	popl %ecx ;				\
	popl %ebx ;				\
	popl %eax ; 

#define SAVE_EFLAGS pushfl

#define RESTORE_EFLAGS popfl

#	Save all floating point registers on the stack.
#       Note! fnsave is used since fxsave requires stack to be aligned
#	to 16 bytes.
#define SAVE_FP_REGS \
	subl  $112, %esp ;\
	fnsave 0(%esp) ;\
	fwait;

#	Restore floating point registers
#define RESTORE_FP_REGS \
	frstor 0(%esp) ;\
	addl $112, %esp;
	
.text

# Make symbols visible for ld
.globl	scheduler_entry
.globl	syscall_entry
.globl	irq0_entry
.globl	fake_irq7_entry
.globl	enter_critical
.globl	leave_critical
.globl	leave_critical_delayed

# This function gets called to enter the scheduler, saving registers
# before doing so.
scheduler_entry:
	# Save regs and eflags
	SAVE_GEN_REGS
	SAVE_FP_REGS
	SAVE_EFLAGS
	movl	current_running, %eax
# Simulate a push to the stack, so that the saved stack will include
# the return address to this function
	subl	$4, %esp
	movl	%esp, PCB_KERNEL_STACK(%eax)
	addl	$4, %esp
	call	scheduler
	RESTORE_EFLAGS
	RESTORE_FP_REGS
	RESTORE_GEN_REGS
	ret

# This is a temporary variable to store the value of %eax when the
# registers are restored.
syscall_return_val:
	.long	0

# Syscalls are entered via this function. We immediately enter a
# critical section, before we switch to the kernel stack. Then
# the registers are saved, before the arguments to the syscall
# are pushed onto the stack, and the syscall is handled. When
# the syscall returns, the arguments are popped off the stack,
# the return value is saved to syscall_return_val (above),
# before the context is restored. Finally, we switch back to the
# user stack, before %eax is given the value we just stored in
# syscall_return_val, and the iret is executed.

syscall_entry:
	# Enter critical section
	call enter_critical
	
	# Switch stack
	call	switch_to_kernel_stack
	
	# Save registers
	SAVE_GEN_REGS
	SAVE_FP_REGS
	
	# Push syscall arguments
	pushl	%ebx	# Arg 1
	pushl	%eax	# Syscall number
	
	# System call helper handles the syscall itself. It will
	# temporarily exit the critical region, but re-enters it before
	# it returns.
	call	system_call_helper
	
	# Pop arguments
	addl	$8, %esp
	
	# Save return value
	movl	%eax, (syscall_return_val)
	
	# Restore registers
	RESTORE_FP_REGS
	RESTORE_GEN_REGS
	# Restore return value (do this before leaving critical, otherwise
	# a race condition may arise)
	movl	(syscall_return_val), %eax
	# Switch to user stack
	call	switch_to_user_stack
	# Leave critical section, and return
	call	leave_critical_delayed
	iret
	
# Timer interrupt entry
irq0_entry:
	# You must add code here to handle the timer interrupt.
	iret

fake_irq7_entry:
  # Add code here to handle the fake irq7 interrupt
	hlt
	
# switch_to_kernel_stack and switch_to_user_stack both switch to its respective
# stack without modifying any registers. Note that interrupts should
# be disabled when calling these functions, as they share space for
# their scratch variables. The stack remains untouched if 
# nested_count > 0. (This implies that we never switch stacks
# when dealing with threads, as they have their nested_count set
# to 1 in _start().)
switch_stack_scratch:
	.long	0
ret_addr:
	.long	0
switch_to_kernel_stack:
	# Save %eax
	movl	%eax, (switch_stack_scratch)
	# Pop return address, so we can return when we have switched stacks
	popl	%eax
	movl	%eax, (ret_addr)
	# Don't switch stack if nested_count != 0
	movl	current_running, %eax
	cmpl	$0, PCB_NESTED_COUNT(%eax)
	jne	switch_return
	# Save user stack, and switch to kernel stack
	movl	%esp, PCB_USER_STACK(%eax)
	movl	PCB_KERNEL_STACK(%eax), %esp
	jmp	switch_return
	
# This basically does the same as switch_to_kernel_stack, except that
# the kernel stack is saved, and we switch to the user stack.
	
switch_to_user_stack:
	movl	%eax, (switch_stack_scratch)
	popl	%eax
	movl	%eax, (ret_addr)
	movl	current_running, %eax
	cmpl	$0, PCB_NESTED_COUNT(%eax)
	jne	switch_return
	# Switch to user stack
	movl	%esp, PCB_KERNEL_STACK(%eax)
	movl	PCB_USER_STACK(%eax), %esp
	jmp	switch_return

# Both s_t_user_stack and s_t_kernel_stack return from this point
switch_return:
	movl	(ret_addr), %eax
	pushl	%eax
	movl	(switch_stack_scratch), %eax
	ret

# enter_critical, leave_critical and leave_critical_delayed:
# These functions enter and leave a critical region by disabling and
# enabling interrupts. Interrupts are only enabled if disable_count == 0.
# Note that they do no registers are modified! Also note that
# leave_critical_delayed requires that disable_count == 0 (if not, the
# computer is halted).
	
enter_critical:
	cli
	incl	disable_count
	ret
	
leave_critical:
	decl	disable_count
	cmpl	$0, disable_count
	jne	leave_critical_return
	sti
leave_critical_return:
	ret
	
leave_critical_delayed:
	decl	disable_count
	cmpl	$0, disable_count
	jne	leave_crit_delayed_err
	ret
leave_crit_delayed_err:
	call	exception_1
	# Never reached
	hlt
	
