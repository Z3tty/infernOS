#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"
#include "kernel.h"
#include "thread.h"

/* Constants */
enum {
  RUNNING = 0,
  BLOCKED,
  SLEEPING,
  EXITED,


  /*
   * interrupts enabled, I/O privilege level 0 (only kernel mode
   * processes/threads can do I/O operations)
   */
  INIT_EFLAGS = ((0 << 12) | (1 << 9))
};

/* Macros */
#ifdef DEBUG
extern int critical_count;
/*
 * The TRACE macro allows you to trace critical sections as they are
 * entered and exited. Currently, you'll need to do some hacking to
 * make it work, as the enter/leave_critical function in entry.S don't
 * honor this macro. (A possibility is to implement the enter/leave
 * functions in C, and have enter/leave_critical call their C
 * implementation.  Take care to save the registers, though!)
 */
#define TRACE(s)      				\
  do {    						\
  scrprintf(critical_count % 10 + 2, 0, "%d", critical_count / 10);  \
  scrprintf(critical_count % 10 + 2, 10, "%s", (s));    	\
  scrprintf(critical_count % 10 + 2, 20, "%s", __FILE__);    \
  scrprintf(critical_count % 10 + 2, 35, "%d", __LINE__);    \
  scrprintf(critical_count % 10 + 2, 45,  "%d",  		\
    current_running->disable_count);  		\
  scrprintf(critical_count % 10 + 2, 50, "%d", current_running->pid);  \
  scrprintf(critical_count % 10 + 2, 55, "%d",    		\
    current_running->nested_count);  			\
  critical_count++;    				\
  } while (0)
#else
#define TRACE(s)
#endif /* DEBUG */

/* Restore stack pointer from s */
#define RESTORE_STACK(s) \
  asm volatile ("# restore stack"); \
  asm volatile ("movl %0, %%esp"::"q" (s))

/* Execute a 'ret' instruction */
#define RETURN_FROM_PROCEDURE \
  asm volatile ("ret")

/* Execute an 'iret' instruction */
#define RETURN_FROM_INTERRUPT \
  asm volatile ("iret")

/* Push x on the stack */
#define PUSH(x) \
  asm volatile ("pushl %0"::"q" (x))

/* Pop something from the stack and save it in x */
#define POP(x) \
  asm volatile ("popl %0":"=q" (x))

/* Disable interrupts unconditionaly. */
#define CLI()  asm( "cli" )

/* Enable interrupts unconditionaly. */
#define STI()  asm( "sti" )


/*
 * Disable interrupts and return the value of he EFlags register prior
 * to disabling the interrupts.  The returned value may be used as an
 * argument to STI_FL() to enable the interrupts only if the
 * interrupts was enabled prior to invoking CLI_FL().
 */
#define CLI_FL()      				      \
({      						      \
  long eflags;    					      \
  __asm__ volatile ("pushfl           # Store EFlags on stack\n"        \
        "popl   %0        # Store Eflags in general reg.\n" \
        "cli              # Disable interrupts          \n" \
        : "=r"(eflags) );				      \
  eflags;    						      \
})


/* Enable interrupts if the IF bit in the indicated EFlags value is set. */
#define STI_FL(eflags)      				     \
  __asm__ volatile ("btl $9,%0     # Check IF bit in stored EFlags \n" \
        "jnc 1f        # If bit == 0, just return      \n" \
        "sti           # If bit == 1, enable int.      \n" \
        "1:                                            \n" \
        :						     \
        :"rm"(eflags))


/* Prototypes */

/* Calls scheduler to run the 'next' process */
void yield(void);

/* Save context and kernel stack, before calling scheduler (in entry.S) */
void scheduler_entry(void);

/*
 * Figure out which process should be next to run, and remove current
 * process from the ready queue if the process has exited or is
 * blocked (ie, cr->status == BLOCKED or cr->status == EXITED).
 */
void scheduler(void);
/* Dispatch next process */
void dispatch(void);

/*
 * Set status = EXITED and call scheduler_entry() which will remove
 * the job from the ready queue and pick next process to run.
 */
void exit(void);

/* Return process id of current process */
int getpid(void);
/* Get and set current process' priority */
int getpriority(void);
void setpriority(int);

/* Returns the current value of cpu_mhz */
int cpuspeed(void);

/*
 * Remove current running from ready queue and insert it into 'q', and
 * release <spinlock> (don't touch <spinlock> if it is 0!)
 */
void block(pcb_t ** q, spinlock_t *spinlock);

/* Move first process in 'q' into the ready queue */
void unblock(pcb_t ** q);


/* Read the directory from the USB stick and copy it to 'buf' */
int readdir(unsigned char *buf);

/* Load a process from the USB stick */
void loadproc(int location, int size);

/* Remove pcb from its current queue and insert it into the free_pcb queue */
void free_pcb(pcb_t * pcb);

#endif /* !SCHEDULER_H */
