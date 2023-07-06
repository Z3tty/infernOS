# Project 2: Non-Preemptive Scheduling

INF-2201: Operating Systems Fundamentals,
Spring 2023
UiT, The Arctic University of Norway
 
 ## Assignment
 
With the boot mechanism built in the first project, we can start developing an operating system kernel. The goal of this project is to design and implement a simple multiprogramming kernel with a non-preemptive scheduler. Although the kernel will be very primitive, you will be dealing with several fundamental mechanisms and apply techniques that are important for building operating systems supporting multiprogramming.

The kernel of this project and future projects supports both processes and threads. Note that the processes and threads in this project are a bit different from those in traditional operating systems (like Unix). All processes, threads and the kernel share a flat address space. The main difference between processes and threads in this project is that a process resides in a separate object file so that it cannot easily address data in another process (and the kernel), whereas threads are linked together with the kernel in a single object file. Letting each process reside in its own object file provides it with some protection. In the future, we will develop support for processes with separate address spaces.

From now on, the kernel will use the protected mode of the x86 processors instead of the real mode. In this project the separation between kernel mode and user mode is only logical, everything runs in the processors highest privilege level (ring 0).

You are required to do the following: 
1. Initialize processes and threads, eg. allocating stacks and setting up process control blocks (PCB).
2. Implement a simple non-preemptive scheduler such that every time a process or thread calls yield or exit , the scheduler picks the next thread and schedules it. As discussed in the class, threads need to explicitly invoke one of these system calls in order to invoke the scheduler, otherwise a thread can run forever.
3. Implement a simple system call mechanism. Since all code runs in the same address space and privilege mode, the system call mechanism can be a simple jump table. But the basic single-entry point system call framework should be in place. You are required to implement two system calls: yield and exit.
4. Implement two synchronization primitives for mutual exclusion: lock acquire and lock release . Their semantics should be identical to that described in Birrell's paper. To implement mutual exclusion, the scheduler needs to support blocking and unblocking of threads. This should be implemented as the two functions block and unblock , but they are not system calls and should only be called from within the Kernel.
5. Measure the overhead of context switches.

**Extra Credits**

All assignment requirements noted above counts for a total of 100%. You may earn additional score by completing the extra credit features noted below. These are by no means mandatory, but may prove helpful achieving top grade in this course. Each feature yields the same score for completion.

1. To receive an extra credit, your implementation should keep track of user and system time used by each thread and process and print it to the screen (much like 'time' on a Unix system, when a process exits). The time should be be measuring in clock cycles using the get timer function that you also used to measure context switches.
2. Add a fourth thread to the kernel.
3. Add a third process to your OS.

## Pre-code

Detailed explanation of each file provided in the precode. All filenames in **bold** require modification for this assignment.
| File 	        | Description |
----------------|-------------|
| Makefile      |	Simplify building the collection of files into an image |
| bochsrc 	     | A configuration file for bochs | 
| bootblock.s   |	Code for the bootblock of a bootable disk |
| common.h      |	Common definitions between the kernel and user processes | 
| createimage.c |	Tool to create a bootable operating system image of the ELF-formated files provided to it |
| **entry.S**   |	Template for you to write assembly code for the kernel entry |
| **kernel.c**  | Kernel code. Contains the \_start initial function |
| **kernel.h**  | Header file for kernel.c |
| **lock.c** 	  | Template for the implementation of synchronization primitives |
| **lock.h** 	  | Header files for lock.c |
| process1.c    | The first user process. A simple plane flying across the screen |
| process2.c    | The second user process. A simple incremental counting process |
| **scheduler.c**	| Scheduler code. Responsible for deciding what PCB to run |
| **scheduler.h** | Header file for scheduler.c |
| syslib.c 	    | System call interface, linked with user processes |
| syslib.h 	    | Header file for syslib.c |
| th.h 	        | Header file for th1.c, th2.c, and th3.c |
| th1.c 	       | Code for the loader and clock thread |
| th2.c 	       | Code of two kernel threads to test synchronization primitives |
| th3.c 	       | Implementation of the Monte Carlo Pi calculation |
| util.c        |	Library for useful support routines |
| util.h 	      |Header file for util.c |

## Detailed Requirements and Hints

You need to understand the PCB and figure out what you need to store in it. The PCB provided only has fields for the previous and next process, the rest is up to you. Since processes and threads share the same address space in this project, the PCB can be very simple.

In the template file kernel.c, you need to implement the  start function to initialize the processes and threads you wish to run. For example, you need to know at which location in memory the code for a process or a thread starts and where the stack for that process or thread is. Once the required information has been recorded in the appropriate data structures you will need to start the first process or thread. Note that a process is placed at a fixed location in memory. This location is different for each process. You can figure this out by looking in the Makefile.

The system call mechanism should use a single entry point as discussed in the classes. The main difference is that you are not using the int instruction to trap into the kernel, but simply make a normal procedure call instead. In order to make this mechanism work, you need to understand the stack frame and the calling convention.

The synchronization primitives are meant for threads only and need not be implemented as system calls. They need to deal with the PCB data structures to block and unblock threads. An important step is to figure out how to queue blocked threads. Although it is easy to implement the synchronization primitives in the way that would works with a preemptive scheduler, it is not required in this project.

Finally, once all this is up and running you should measure the overhead (time used) of context switches in your system. To do this you can use the provided function get timer(). This function contains inline assembly that returns the number of clock cycles that have passed since the processor was booted. Note that the value returned is 64 bit (unsigned long long int). This function will only run on Pentium or newer processors. Also, notice that exact measurements can only be done when running on real hardware. 

## Design review

For the design review, you need to have figured out all the details including the PCB data structure, the system call mechanism, function prototypes, how to switch from one thread to another and so on. In other words, you should have a design that is ready to be implemented.

Before you come to meet the reviews, you should prepare a design document. This document should contain detailed descriptions of the various parts of your design. An oral only presentation is not acceptable. 

## Handin

This project is an **home-exam**. Your deliverable will be graded and count towards the final grade for this course. The handin is in Wiseflow. 

You should handin:
1. A report, maximum 4 pages that gives overview of how you solved each task (including extra credits), how you have tested your code, and any known bugs or isseues. In addition you need to describe the methodology, results, and conclusions for your performance measurements.
    * The report should include your name, e-mail, and GitHub username.
    * The four page limit includes everything.
    * The report shoubd be in the doc directory in your repository.
3. Your code in a zip file.

## Useful resources
1. [An Introduction to Programming with Threads](birrel.pdf), Andrew D. Birrell
2. [GCC Inline Assembly Howto](http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html)
3. [Precept slides](P2-precepts.pptx)
4. [Protected mode - a brief introduction](protected-mode.pdf), Erlend Graff
