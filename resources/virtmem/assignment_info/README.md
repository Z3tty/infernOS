
# Project 5: Virtual Memory

 INF-2201: Operating Systems Fundamentals

UiT The Arctic University of Norway 


## Assignment

In this project you will implement a simple **demand-paged virtual memory system** using an USB stick as the swapping area. Each process should have its own separate *virtual address space*, with proper protection so that a process may not access or modify data belonging to another process. Additionally the kernel should also be protected from the processes in a similar manner.

You will need to do the following:
1. Set up two-level page tables per process, which should also include the entire address space of the kernel. Thus, all kernel threads share the same page table.
2. When page faults happen you will need a *page fault handler* to allocate a physical pages and fetch virtual pages from disk.
3. Implement on a *physical page allocation policy* to allocate physical pages. When it runs out of free pages, it should use random page replacement to evict a physical page to the disk.

## Extra credit

All assignment requirements noted above counts for a total of 100%. You may earn additional score by completing the extra credit features noted below. These are by no means mandatory, but may prove helpful achieving top grade in this course. Each feature yields the same score for completion.

1. Implement a FIFO replacement policy. You can expand even more by implementing a FIFO with second chance replacement policy.
2. Implement a swap area on the disk, allowing multiple copies of the same program to be run. (See the section Detailed Requirements and Hints below for simplified design and why this is extra credit.)

## Detailed explanation of the precode

In this project the kernel and the kernel threads run in *kernel mode* (privilege level 0) and the processes are executed in *user mode* (privilege level 3).

We have provided user and kernel protection mechanisms, which will aid you in implementing the virtual memory system.

## Provided files

Detailed explanation of each file provided in the precode. All filenames in **bold** require modification for this assignment. 

| File 	        | Description |
|---------------|-------------|
| Makefile 	    | Simplify building the collection of files into an image. | 
| asmdefs.c 	   | Tool to create header file containing PCB struct offsets. The offsets can be used in assembly code to access the different fields in a struct. |
| bochsrc 	     | Configuration for Bochs, version greater than 2.5 |
| bootblock.S  	| Code for the bootblock of a bootable disk. |
| common.h 	    | ommon constants, macros and type definitions. |
| createimage.c | Tool to create a bootable operating system image of the ELF-formated files provided to it. |
| dispatch.h   	| Header file for dispatch.S |
| dispatch.S 	  | Assembly stub to dispatch a newly scheduled process/thread. |
| entry.S 	     | Low-level interrupt handler code, used to enter the kernel. |
| interrupt.c   | Interrupt handlers. |
| interrupt.h 	 | Header file for interrupt.c |
| kernel.c 	    | Kernel code. Contains the \_start initial function. |
| kernel.h 	    | Header file for kernel.c |
| keyboard.c 	  | Keyboard handling code. |
| keyboard.h 	  | Header file for keyboard.c |
| mbox.c 	      | Implementation of the mailbox interprocess communication primitive. |
| mbox.h 	      | Header file for mbox.c |
| **memory.c**  | Allocator for physical memory page frame, page-table handling and virtual memory mapping. |
| **memory.h**  | Header file for memory.c |
| print.c 	     | Implements printing to a generic output device(screen/serial). |
| print.h 	     | Header file for print.c |
| process1.c    |	The first user process. A simple plane flying across the screen. |
| process2.c    |	The second user process. A simple incremental counting process. |
| process3.c 	  | One of two processes that communicate using the mailbox IPC mechanism. |
| process4.c 	  | One of two processes that communicate using the mailbox IPC mechanism. |
| proc_start.s 	| Assembly stub to reliably enter and exit user processes. |
| scheduler.c 	 | Scheduler code. Responsible for deciding what PCB to run. |
| scheduler.h 	 | Header file for scheduler.c |
| screen.h 	    | Screen coordinates. |
| SeaBIOS.bin 	 | Emulated BIOS used by Bochs. |
| shell.c 	     | A simple shell that uses the keyboard input. Use the shell to load user processes. |
| sleep.c 	     | Millisecond-resolution sleep function. Has suuport in the scheduler to avoid busy-wait. |
| sleep.h 	     | Header file for sleep.c |
| syslib.c 	    | System call interface, linked with user processes. |
| syslib.h 	    | Header file for syslib.c |
| th.h 	        | Header file for th1.c and th2.c |
| th1.c 	       | Code for the loader and clock thread. |
| th2.c 	       | Code of two kernel threads to test synchronization primitives. |
| thread.c 	    | Implementation of synchronization primitives; mutexes, conditional variables, semaphores and barriers. |
| thread.h 	    | Header file for thread.c |
| time.c        |	Contains routines that detect the CPU speed. |
| time.h 	      | Header file for time.c |
| tlb.h 	       | Header file for tlb.S |
| tlb.S 	       | Assebmly stub to flush the Translation Lookaside Buffer. |
| usb/*.*       | Implementation of the USB 1.1 drivers. |
| util.c 	      | Library for useful support routines. |
| util.h 	      | Header file for time.c |
| VGABIOS.bin 	 | Emulated VGA BIOS support for Bochs. |

## Detailed Requirements and Hints

Before starting on this project, you should read chapters 13 and 14 of Protected Mode Software Architecture carefully (alternatively Chapters 2-4 of the [Intel Architecture Software Developer's Manual, Volume 3: System Programming Guide](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html)) to understand the virtual address translation mechanism needed to set up page tables. As described in the manuals, the x86 architecture uses a two-level page table structure with a directory that has 1024 entries pointing to the actual page tables. You need to understand this structure very well.

You can make the following assumptions and constraints to simplify your design and implementation:
* The amount of memory (number of page frames) available for paging has to be limited so that the memory requirements of your modest number of processes actually triggers paging. The provided code suggest limiting the number of pages to 33. This value may need tuning depending on your design.
* You can use the area allocated on the disk for the image file as the backing store for each process.
* You may assume that processes do not use a heap for dynamic memory allocation.
* You need to figure out how to initially map your physical memory pages to run kernel threads and how to load code and data of a process into its address space. When the address space for a process is initialized, all code and data pages for the process should be marked as invalid. This triggers the page fault handler to be invoked on the first access to any of these pages.
* Finally, the page fault handler may deal with only data and code pages. All other pages, like user stack and page tables could be "pinned" so they will never be swapped out of main memory. This means that you need to perform pinning when you initialize a process's address space.

Useful Resources:
* Protected Mode Software Architecture, chapters 7–12 describe segment and protection mechanisms
* Protected Mode Software Architecturem, chapters 13 and 14 describe virtual memory
* Chapter 2 (System Architecture Overview), chapter 3 (Protected-Mode Memory Management), and chapter 4 (Protection), Intel Architecture Software Developer's Manual, Volume 3: System Programming Guide

## Handin

This project is a mandatory assignment. The handin is in GitHub.

You should commit:
* A report, maximum 4 pages that gives overview of how you solved each task, how you have tested your code, and any known bugs or issues.
    * The report should include your name, e-mail, and GitHub username.
    * The four page limit includes everything.
    * The report shoubd be in the doc directory in your repository.
* Your code in your private repository.
