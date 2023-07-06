
# Project 4: Inter-Process Communication and Process Management 

 INF-2201: Operating Systems Fundamentals

UiT The Arctic University of Norway 


## Assignment

This project consists of two separate parts: message passing mechanism, and process management.

In this project, we introduce the user mode: processes have their own address spaces, and are locked from accessing kernel memory. For this, the OS employs the segmenting and paging functionalities Intel provides with its CPUs, and the CPUs protection features. You do not need to know understand memory segmenting and paging yet, but you may browse the source code since the next assignment will deal with Virtual Memory. 

## Message passing

Message passing allows processes to communicate with each other by sending and receiving messages. We will add support for the keyboard using message passing.

You are required to do the following for this part of the assignment:

Implement a **mailbox** mechanism as the message passing mechanism. You will have to implement 6 functions, of which 5 are system calls. The syntax of these primitives can be found in the header file *mbox.h*.

| Function name |	Description |
|-------------|---------------|
| mbox_init 	   | Initializes the data structures of all mailboxes in the kernel |
| mbox_open 	   | Opens a mailbox for either mbox_send or mbox_recv |
| mbox_close 	  | Closes a mailbox. When no process uses the mailbox, its data structure will be reclaimed |
| mbox_send 	   | Sends a message to the mailbox |
| mbox_recv 	   | Receives a message from a mailbox |
| mbox_stat 	   | Returns the number of messages in the mailbox |
 
 Implement 3 functions to support **keyboard input** (in *keyboard.c*)

| Function name	| Description |
|-------------|--------------|
| keyboard_init |	Initializes the data structures for managing keyboard input |
| getchar 	     | Gets the next keyboard character |
| putchar 	     | Called by the keyboard interrupt handler to put a character in the keyboard buffer |

We want interrupts to be disabled as infrequently as possible, and this project requires you to **improve the implementation of mutexes and condition variables** to achieve this. You need to add code to the file *thread.c* only.

## Process management

This will make our OS look more like a real one. We will add some preliminary process management into the kernel including dynamic process loading which will enable you to actually run a program from any storage device. Also, for those who always have energy and time, there are plenty of possibilities to do more in this part.

To load a program dynamically from a storage device, we will have to first agree upon a really simple format for a file system. Our file system is flat and of limited size. It's flat, because there is no directory structures. All files reside in the same root directory. It's of limited size because only one sector will be used as the root directory, and therefore only a limited number of entries can be stored there. A more detailed structure of this simple file system is given in the section below.

We want to be able to tell the OS to "load" a program from a program disk. The kernel will then read the root directory sector of the disk and find the offset and length of the program, read it into a memory area which is not used, and then allocate a new PCB entry for it. You need to allocate memory for the process, but you do not need to worry about the deallocation of memory when a process exits. (If you have the time and energy to do more you can actually deallocate all the memories used by a process when it exits.)

You can use createimage with the "cantboot" fake bootsector to create an image that contains only programs and no kernel. Look at the Makefile target "progdisk" for details. After booting your OS, you should be able to switch sticks and then load programs from a stick that contains only programs.

You will need to implement the following:

| Function name |	Description |
|---------------|--------------|
| loadproc 	    | Loads the ith program on the program disk, creates a new PCB entry and inserts it into the ready list |
| readdir 	     | Reads the program directory on the floppy into a given buffer (just read a whole sector) |

## Extra credits

 All assignment requirements noted above counts for a total of 100%. You may earn additional score by completing the extra credit features noted below. These are by no means mandatory, but may prove helpful achieving top grade in this course. 

1. **Memory reclamation**: After a process exits or gets killed (if you implement kill, see below), you should free all the memory the process uses, and the PCB entry as well, so that the next newly started process can reuse this memory. You can use any simple (and good) management algorithm, such as First Fit or Best Fit. Also, you can have a fixed-size PCB table in your kernel, so that the user can create no more than this number of processes. Note: You must implement your own memory manager and a malloc-like and a free-like functions. You can't use the standard malloc() function in C library, because we are not linking against any standard libraries.
2. **Better memory management**: You can allocate each PCB entry dynamically, so that the only limit on the number of processes is the available memory.
3. **PS**: Implement a UNIX-like ps command, so that the user can type "ps" in the shell and view all the processes along with their pids, statuses, and/or any other information you can come up with.
4. **KILL**: Implement a UNIX-like kill command, so that the user can type "kill" in the shell and kill a process specified by its pid. Note: It's okay to not be able to kill a thread, because our threads are linked statically with the kernel. You should take care of locks and condition variables that a process being killed owns. You don't really want to kill a process that already acquires 10 locks. To solve this problem, you might use "delayed killing". When you kill a process you only set its status to KILLED, and the scheduler will allow it continue to run until you are sure that all the locks it owns are release by it. A little hint: the only way that a process can access a lock is by calling mbox system calls.
5. **Suspend/Resume**: Be able to suspend and resume a process.

## Do even more

A cool game: Create a character-based game, such as Tetris or Breakout.

Or a more advanced shell.

## Provided files

Detailed explanation of each file provided in the precode. All filenames in **bold** require modification for this assignment.
| File 	        | Description |
|---------------|-------------|
| Makefile 	    | Simplify building the collection of files into an image | 
| asmdefs.c     |	Tool to create header file containing PCB struct offsets. The offsets can be used in assembly code to access the different fields in a struct. |
| bootblock.S   |	Code for the bootblock of a bootable disk |
| cantboot.s   	| Code for the bootblock of a non-bootable disk| 
| common.h 	    | Common constants, macros and type definitions| 
| createimage.c | Tool to create a bootable operating system image of the ELF-formated files provided to it |
| dispatch.h 	  | Header file for dispatch.S |
| dispatch.S 	  | Assembly stub to dispatch a newly scheduled process/thread |
| entry.S 	     | Low-level interrupt handler code, used to enter the kernel | 
| interrupt.c 	 | Interrupt handlers |
| interrupt.h 	 | Header file for interrupt.c |
| **kernel.c**  | Kernel code. Contains the \_start initial function |
| kernel.h 	    | Header file for kernel.c |
| **keyboard.c** | Keyboard handling code |
| keyboard.h 	  | Header file for keyboard.c |
| **mbox.c**    | Implementation of the mailbox interprocess communication primitive |
| mbox.h 	      | Header file for mbox. |
| memory.c 	    | Allocator for physical memory page frame, page-table handling and virtual memory mapping |
| memory.h 	    | Header file for memory.c |
| paging.c 	    | Enables simple paging for user mode |
| paging.h 	    | Header file for paging.c |
| print.c 	     | Implements printing to a generic output device(screen/serial) |
| print.h 	     | Header file for print.c |
| process1.c 	  | The first user process. A simple plane flying across the screen. |
| process2.c 	  | The second user process. A simple incremental counting process. |
| process3.c 	  | One of two processes that communicate using the mailbox IPC mechanism |
| process4.c 	  | One of two processes that communicate using the mailbox IPC mechanism |
| proc_start.s 	| Assembly stub to reliably enter and exit user processes |
| scheduler.c  	| Scheduler code. Responsible for deciding what PCB to run |
| scheduler.h 	 | Header file for scheduler.c |
| screen.h 	    | Screen coordinates |
| shell.c 	     | A primitive shell that uses the keyboard input. Use the shell to load user processes |
| sleep.c 	     | Millisecond-resolution sleep function. Has suuport in the scheduler to avoid busy-wait |
| sleep.h 	     | Header file for sleep.c |
| syslib.c 	    | System call interface, linked with user processes |
| syslib.h 	    | Header file for syslib.c |
| th.h 	        | Header file for th1.c and th2.c |
| th1.c 	       | Code for the loader and clock thread |
| th2.c 	       | Code of two kernel threads to test synchronization primitives |
| **thread.c**  | Implementation of synchronization primitives; mutexes, conditional variables, semaphores and barriers |
| thread.h 	    | Header file for thread.c |
| time.c 	      | Contains routines that detect the CPU speed |
| time.h 	      | Header file for time.c |
| usb/*.* 	     | Implementation of the USB 1.1 drivers |
| util.c 	      | Library for useful support routines |
| util.h 	      | Header file for time.c |

## Demo

After you get the sources, try "make demo", or move the file given/image to your USB stick and boot it. 4 threads will be loaded at first, then the loader thread will load the shell. After the shell is running, you can try "ls" in it. You will see 5 process images. Process 0 is the shell itself, so don't load it again (or funny things will happen). Type "load 1", and after the loading finishes you will see a plane flying. Then you may try the command "fire" in the shell. Also, you can load 2, 3, and 4. Note that both 3 and 4 must be loaded to work as intended because they communicate with each other using mail boxes.

## Given solution

The *given* directory contains a bootable image file with a working solution. Boot this image in bochs and/or hardware to get a visual understanding of what you will achieve. It will start by loading 4 threads. One of these is the loader thread, which will load the shell. After the shell is up and running, you can try issuing commands to it. If you issue the "ls" command, you will see 5 process images. Process 0 is the shell itself, and is the one automatically loaded by the loader thread. Type "load 1" will initiate the loading of process 1, and after this finishes the plane will start running. Now you may for instance issue the command "fire", and see what happens! You may also load process 2, 3, and 4. Note that both process 3 and 4 must be loaded to work as intended, because they communicate with each other using mail boxes.

## Detailed Requirements and Hints

To minimize your effort of debugging, we strongly suggest you first finish the **message passing** part before dealing with **process management**, because they are independent.

To **minimize interrupt disabling** in the implementation of mutexes and condition variables, you should use the the spinlock primitives provided to perform atomic operations. Only when you make a call to a scheduler function (like block and unblock) should you turn the interrupts off.

You will be implementing a simple **mailbox mechanism** that supports variable sized messages. All messages will be delivered in FIFO order. So, the implementation of the mailbox mechanism essentially involves solving the bounded buffer problem discussed in the classes. You can use a Mesa-style monitor to implement the bounded buffer.

Adding **keyboard support** requires writing an interrupt handler and a getchar() system call. You can treat the interrupt handler as the producer and the processes calling getchar() as the consumers. The interrupt handler simply turns an input character into a message and places it into the proper mailbox, as discussed in the class. The getchar() system call can call mbox_recv() from the mailbox to read the input character.

The **interrupt handler** needs to handle several interrupts because you have multiple types of interrupts (timer and keyboard, for example). The code in interrupt.c takes care of this, but you should read the code carefully and understand what it is doing.

At this point, you should be able to run your OS. Everything should be running, and running smoothly. For instance, the plane animation should fire a bullet when you type "fire" in your shell while the other threads and processes continue to have progress.

Now you can start doing **process management**. In the simple file system one block is used to hold the process directory. This directory contains one entry per process. Each entry records the offset and size of a process. As shown in the figure below, the disk layout consists of a bootblock, zero or one kernel image, the process directory and zero or more process images.

| Sector |	Description |
|--------|-------------|
| 1 	    | Bootblock
| 2 to os_size+1 | Kernel image (if present) |
| os_size+2 	    | Process directory |
| os_size+3 	    | Process 1 |
| n 	            | Process i |

To make your life a little bit easier, we will give you the solution object files of those four source files that you will be modifying, namely, given/mbox.o, given/keyboard.o, given/thread.o, and given/kernel.o. You can use these given object files to test your program incrementally.
- To test your IPC code, type "make ipc", given/thread.o and given/kernel.o will be used with your version of mbox.o and keyboard.o.
- To test your interrupt disabling code, type "make int".
- To test your process management code, type "make pm".
 - If you modify more than these four files, you will have to make all, or create your own make target. This will be the case when you are doing the extra features.
 - To add user programs, edit the Makefile, and put your user program names after the definition "USER_PROGRAMS = ", then you can type "make progdisk" to make a program disk. Of course, you will have to compile and link your user program separately, or you will have to add some targets to the make yourself.
 - To run a demo which uses a complete working image we provide you, boot the image found in given/image. Use this to see how things are supposed to look like when you are done.

## Handin

This project is an **home-exam**. Your deliverable will be graded and count towards the final grade for this course. The handin is in Wiseflow.

You should handin:
* A report, maximum 4 pages that gives overview of how you solved each task, how you have tested your code, and any known bugs or issues.
    * The report should include your name, e-mail, and GitHub username.
    * The four page limit includes everything.
    * The report shoubd be in the doc directory in your repository.
* Your code in your private repository.
