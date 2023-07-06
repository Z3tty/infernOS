
# Project 6: File System

 INF-2201: Operating Systems Fundamentals

UiT The Arctic University of Norway 


## Assignment

In this project, you will implement a UNIX-like but greatly simplified file system in your operating system. The file system will reside on the USB stick.

You are required to implement the system calls noted with their description in the table below. Most of these are standard UNIX system calls and you can look up man pages for descriptions (the system calls don't have the "fs_" prefix on UNIX). You will be implementing simpler versions of these functions (for instance, there is no notion of permissions/users in this file system).

The shell has been modified so that you can access many of these functions from the prompt. We have also added a few commands like "ls" to the shell, see the source code in shell.c for a complete list.

| Function name |	Description |
|---------------|-------------|
| fs_init 	     | Initialize the file system, e.g., the file descriptor table |
| fs_mkfs 	     | Make a new file system, i.e., format a disk |
| fs_mount 	    | Mount an existing file system |
| fs_umount 	   | Umount a mounted file system |
| fs_open 	     | Open a file with given flags |
| fs_close 	    | Close a file |
| fs_read      	| Read some bytes from a file into a buffer |
| fs_write 	    | Write some bytes to a file from a buffer |
| fs_lseek 	    | Move the file pointer to a new location |
| fs_mkdir 	    | Create a sub-directory under the current directory |
| fs_rmdir 	    | Remove a sub-directory |
| fs_chdir 	    | Change the current directory |
| fs_link 	     | Create a link to a file |
| fs_unlink 	   | Remove a link to a file |
| fs_stat 	     | Get the status of a file |

## Testing on Linux

You can debug your file system in Linux. We have provided enough functions so that you can test your entire implementation on Linux, using a file to emulate the disk. Once it is working, you can move it to our OS. This way you can use all debugging tools on Linux for this project easily.

In addition, we provide a few files which can be used to test your code on linux
- util_sim.c : Fake some helper functions for the shell
- block\_sim.c : Uses a UNIX file instead of a disk
- test.py : Python script that runs test commands on the linux shell simulator.

You can test your code on linux by typing "make shell_sim" and executing "./shell_sim". This will use a seperate image represented as one file that will be created during this compilation process.

## Extra Credit

All assignment requirements noted above counts for a total of 100%. You may earn additional score by completing the extra credit features noted below. Each feature yields the same score for completion.

1.  n-level (n>1) inode block lists.
2.  A file system cache.
3.  A consistent file system cache.
4.  Processes and kernel as files. 

Additional details are in the precept slides.

## Provided files

Detailed explanation of each file provided in the precode. All filenames in **bold** require modification for this assignment. 

| File     | 	Description |
|---------------|-----------|
| Makefile   	  | Simplify building the collection of files into an image. |
| asmdefs.c     |	Tool to create header file containing PCB struct offsets. The offsets can be used in assembly code to access the different fields in a struct. |
| **block.c**   | Read/write a block of the file system to stable storage. |
| block.h 	     | Header file for block.c and block_sim.c |
| block_sim.c   |	Simulates the block operations on Linux. |
| bootblock.S   |	Code for the bootblock of a bootable disk. |
| common.h 	    | Common constants, macros and type definitions. |
| createimage.c |	Tool to create a bootable operating system image of the ELF-formated files provided to it. |
| dispatch.h    |	Header file for dispatch.S |
| dispatch.S    |	Assembly stub to dispatch a newly scheduled process/thread. |
| entry.S 	     | Low-level interrupt handler code, used to enter the kernel. |
| **fs.c**      | File system calls - Implements the functions related to file system operations. |
| fs_error.c 	  | File system error codes, and function to print the corresponding error code as a text string. Works in both this OS and in the Linux simulation. |
| fs_error.h 	  | Header file for fs_error.c |
| fs.h 	        | Header file for fs.c |
| fstypes.h 	   | Header file defining a process file descriptor entry structure. |
| inode.h 	     | Header file defining disk and memory inode structure. |
| interrupt.c 	 | Interrupt handlers. |
| interrupt.h   |	Header file for interrupt.c |
| kernel.c 	    | Kernel code. Contains the \_start initial function. |
| kernel.h 	    | Header file for kernel.c |
| keyboard.c 	  | Keyboard handling code. |
| keyboard.h 	  | Header file for keyboard.c |
| mbox.c 	      | Implementation of the mailbox interprocess communication primitive. |
| mbox.h 	      | Header file for mbox.c |
| memory.c 	    | Allocator for physical memory page frame, page-table handling and virtual memory mapping. |
| memory.h 	    | Header file for memory.c |
| print.c 	     | Implements printing to a generic output device(screen/serial). |
| print.h 	     | Header file for print.c |
| process1.c 	  | The first user process. A simple plane flying across the screen. |
| process2.c 	  | The second user process. A simple incremental counting process. |
| process3.c 	  | One of two processes that communicate using the mailbox IPC mechanism. |
| process4.c 	  | One of two processes that communicate using the mailbox IPC mechanism. |
| proc_start.s 	| Assembly stub to reliably enter and exit user processes. |
| scheduler.c 	 | Scheduler code. Responsible for deciding what PCB to run. |
| scheduler.h 	 | Header file for scheduler.c |
| screen.h 	    | Screen coordinates. |
| SeaBIOS.bin 	 | Emulated BIOS used by Bochs. |
| shell.c 	     | A primitive shell that uses the keyboard input. Use the shell to load user processes. |
| shell_sim.c 	 | Linux implementation of the shell for simulation. |
| sleep.c 	     | Millisecond-resolution sleep function. Has suuport in the scheduler to avoid busy-wait. |
| sleep.h 	     | Header file for sleep.c |
| superblock.h 	| Header file defining the disk and memory superblock structure. |
| syslib.c 	    | System call interface, linked with user processes. |
| syslib.h 	    | Header file for syslib.c |
| th.h 	        | Header file for th1.c and th2.c |
| th1.c 	       | Code for the loader and clock thread. |
| th2.c 	       | Code of two kernel threads to test synchronization primitives. |
| thread.c 	    | Implementation of synchronization primitives; mutexes, conditional variables, semaphores and barriers. |
| thread.h 	    | Header file for thread.c |
| thread_sim.c 	| Simulation of thread primitives for Linux. |
| time.c 	      | Contains routines that detect the CPU speed. |
| time.h 	      | Header file for time.c |
| towers_of_hanoi.c |	Process implementation of a solution to towers of hanoi problem. |
| usb/*.* 	     | Implementation of the USB 1.1 drivers. |
| util.c 	      | Library for useful support routines. |
| util.h 	      | Header file for time.c |
| util_sim.c 	  | Linux implementation of the util library used for simulation. |

## Detailed Requirements and Hints

The first thing you should do is to figure out your disk layout: boot block, super block, i-nodes, and data blocks.  You also need to figure out how to manage free sectors. You can use a simple bit-map scheme discussed in class.

You can implement a simplified i-node structure with only direct pointers to disk blocks (implementing indirect pointers gives extra credit).

You need to figure out how to implement directories as well as the attributes for each file.

The requirement of this project is very basic; you need not to worry about permission checking and so on. Also, there is no need to worry about scheduling disk requests.

## Handin

This project is an **home-exam**. Your deliverable will be graded and count towards the final grade for this course. The handin is in Wiseflow.

You should handin:
* A report, maximum 4 pages that gives overview of how you solved each task, **how you have tested your code**, and any known bugs or issues.
    * The report should include your name, e-mail, and GitHub username.
    * The four page limit includes everything.
    * The report shoubd be in the doc directory in your repository.
* Your code.

