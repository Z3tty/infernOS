
# Project 3: Preemptive Scheduling 

 INF-2201: Operating Systems Fundamentals

UiT The Arctic University of Norway 

## Assignment

The previous project, P2, was a simple but useful non-preemptive, multi-threaded operating system kernel. The main goal of the present project, P3, is to transform the non-preemptive kernel into a preemptive kernel. You will also change the system call mechanism from a simple function call to an interrupt based mechanism as used in contemporary operating systems. In addition, you will deal with some concurrent programming issues: you will add the synchronization primitives semaphores, barriers, and condition variables, and you will work on the famous dining philosophers problem.

The OS kernel of this project and future projects supports both processes and kernel threads. See the description for P2 on how the processes and threads in our project differ from the traditional notion of processes and threads.

In this project, everything still runs in the CPU's kernel mode with a flat address space. Also note that the *delay* function has been removed, and replaced with *ms_delay*, which waits the specified number of milliseconds.

You are required to do the following:
1. Develop an interrupt handler that invokes the scheduler. This interrupt will be triggered once every 10 ms by a timer.
2. Although this kernel is preemptive, the *yield* (system) call should still work correctly.
3. Use a software interrupt to perform system calls. All of this code is actually given to you as a template. The system calls (available to processes) that are already implemented are *yield*, *getpid* and *exit*. You are required to ensure that these system calls perform correctly even in the presence of the timer interrupt.
4. The synchronization primitives for mutual exclusion: *lock_acquire* and *lock_release* should continue to work in the presence of preemptive scheduling. Note that these two calls are used only for kernel threads.
5. You need to add semaphore, condition variable and barrier functionalities to the existing synchronization primitives.
6. Implement a fair solution to the dining philosophers, and use condition variables in your implementation. A solution is "fair" when all philosophers can eat for the same amount of time.

## Extra Credit

All assignment requirements noted above counts for a total of 100%. You may earn additional score by completing the extra credit features noted below. These are by no means mandatory, but may prove helpful achieving top grade in this course. Each feature yields the same score for completion.
1. Implement priority scheduling and demonstrate it with the processes provided by us.
2. Re-implement dining philosophers on Linux using the pthreads library.

## Provided files

Detailed explanation of each file provided in the precode. All filenames in **bold** require modification for this assignment. 

| File 	   | Description
-----------|-------------
| Makefile 	      | Simplify building the collection of files into an *image*. |
| asmdefs.c       |	Tool to create header file containing PCB struct offsets. The offsets can be used in assembly code to access the different fields in a struct. |
| barrier_test.c 	| Threads you can use to test your barrier implementation. |
| bootblock.s 	   | Code for the bootblock of a bootable disk. |
| common.h 	      | Common constants, macros and type definitions. |
| createimage.c 	 | Tool to create a bootable operating system image of the ELF-formated files provided to it. |
| dispatch.h 	    | Header file for dispatch.S |
| dispatch.S 	    | Assembly stub to dispatch a newly scheduled process/thread. |
| **entry.S**     |	Template for you to write assembly code for the timer interrupt. |
| interrupt.c 	   | Interrupt handlers. |
| interrupt.h 	   | Header file for interrupt.c |
| kernel.c 	      | Kernel code. Contains the *\_start initial* function. |
| kernel.h 	      | Header file for kernel.c |
| pcb.h 	         | Contains the process control block. |
| pcb_status.h 	  | Constants shared by assembly and C code. |
| **philosophers.c** |	An implementation of a solution for the dining. |
| process1.c 	    | The first user process. A simple plane flying across the screen. |
| process2.c 	    | The second user process. A simple incremental counting process. |
| scheduler.c 	   | Scheduler code. Responsible for deciding what PCB to run. |
| scheduler.h 	   | Header file for scheduler.c |
| screen.h 	      | Screen coordinates. |
| syslib.c 	      | System call interface, linked with user processes. |
| syslib.h 	      | Header file for syslib.c |
| th.h 	          | Header file for th1.c and th2.c |
| th1.c 	         | Code for the loader and clock thread. |
| th2.c 	         | Code of two kernel threads to test synchronization primitives. |
| th3.c 	         | Implementation of the Monte Carlo Pi calculation. |
| **thread.c**   	| Implementation of synchronization primitives; mutexes, conditional variables, semaphores and barriers. |
| **thread.h** 	  | Header file for thread.c. Contains prototypes synchronization primtives. |
| time.c 	        | Contains routines that detect the CPU speed. |
| time.h 	        | Header file for time.c |
| util.c 	        | Library for useful support routines. |
| util.h         	| Header file for time.c |

In addition, you need to create new files for the pthreads extra credit assignemnt. 

## Detailed Requirements and Hints

This project continues using the protected-mode of the CPU and run all code in the kernel-mode flat address space. To implement preemptive scheduling, you need to use the timer interrupt mechanism. We have prepared for you most of the code that is required to set up the interrupt descriptor tables (IDT) and set up the timer chip. You need to understand how it is done so that you can adjust the interrupt frequency. We have provided you with an interrupt handler template. It is your job to decide what goes into the interrupt handler.

You need to figure out where you can perform preemptive scheduling. You need to review the techniques discussed in class. Note that in this project, you can disable interrupts, but you should avoid disabling interrupts for a long period of time (or indefinitely!). In order for processes to make system calls, you will note that the code in syslib.c does not make a function call, rather it invokes a software interrupt. The kernel threads should still work in the same way as before, calling the functions in the kernel directly.

Implement the synchronization primitives in the file thread.c. Everything should work correctly in the presence of preemptive scheduling. You need to review the techniques discussed in class. Note that you can use less complex primitives to implement more complex ones.

To implement a fair solution to the dining philosophers, modify philosophers.c. To get full credits on this part of the project, you must document why your solution is fair (both in theory, and by showing it "live" when your OS runs). You should also use condition variables instead of semaphores. 

## Handin

This project is a mandatory assignment. The handin is in GitHub.

You should commit:
* A report, maximum 4 pages that gives overview of how you solved each task, how you have tested your code, and any known bugs or issues.
    * The report should include your name, e-mail, and GitHub username.
    * The four page limit includes everything.
    * The report shoubd be in the doc directory in your repository.
* Your code in your private repository.

 ## Useful Resources
- Intels manuals, in particular Volume 3A: System Programming Guide, Part 1: Chapter 5: Interrupt and Exception Handling
- [Using Inline Assembly with gcc](http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html)
- Protected Mode Software Architecture, Shanley: Chapter 7, 8, 9 and 12
- [POSIX Threads Programming](https://computing.llnl.gov/tutorials/pthreads/)
