#include "asmsyms.h"

INIT_EFLAGS = ((0 << 12) | (1 << 9))

.global dispatch  

.text
/*
 * Note: Dispatch should only be called within a critical section/with 
 * interrupts off. It checks that disable_count != 0.
 */  
  
dispatch:
  call	setup_current_running
  movl	current_running, %eax
  movl	PCB_KERNEL_STACK(%eax), %esp
  cmpl	$1, PCB_FIRST_TIME(%eax)
  je	first_time
  ret
first_time:  	
  movl	$0, PCB_FIRST_TIME(%eax)
  cmpl	$1, PCB_IS_THREAD(%eax)
  je	isathread
  pushl	PCB_DS(%eax)
  pushl	PCB_USER_STACK(%eax)
isathread:  
  pushl	$INIT_EFLAGS
  pushl	PCB_CS(%eax)
  pushl	PCB_START_PC(%eax)
  call	leave_critical
  movw	PCB_DS(%eax),%ds
  movw	PCB_DS(%eax),%es
  iret
