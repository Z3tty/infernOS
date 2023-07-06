#include "asmsyms.h"
#include "pcb_status.h"	

.global dispatch	
.text
	
dispatch:
	movl	current_running, %eax
	movl	PCB_KERNEL_STACK(%eax), %esp
	cmpl	$STATUS_FIRST_TIME, PCB_STATUS(%eax)
	je	first_time
	ret
first_time:		
 	movl	$STATUS_READY, PCB_STATUS(%eax)
	cmpl	$1, PCB_IS_THREAD(%eax)
	je	isathread
	movl	PCB_USER_STACK(%eax), %esp		
isathread:	
	call	leave_critical
	jmp	*PCB_START_PC(%eax)

