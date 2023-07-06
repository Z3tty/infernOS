#	entry.S
#	This file contains the code for kernel_entry, as well as
#	scheduler_entry, which saves the context of the current process
#	before calling the scheduler. The file is pre-processed before it
#	is assembled, allowing (among other things) the use of macros
#	and C-style comments.

.data
	.align 4
.text
.code32
	.align 4

# Make symbols visible for ld
.globl	kernel_entry
.globl	scheduler_entry


# void kernel_entry(int fn)
#
# SYSCALL(i) in syslib.c calls this function
# fn = system call number (defined in kernel.h)
#
ke_scratch:
	.long	0
ke_arg:
	.long	0
kernel_entry:
	# Save eax
	movl	%eax, (ke_scratch)
	
	# Save argument
	movl	4(%esp), %eax
	movl	%eax, (ke_arg)

	# %eax = pcb_t *current_running;
	movl current_running, %eax
	
	# Now it's up to you!
loop:
	jmp		loop

#	This function gets called to enter the scheduler, saving registers before doing so.
scheduler_entry:
	# Fill in the blanks!
	jmp	scheduler_entry
