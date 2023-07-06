# Project 1: Bootup Mechanism 

INF-2201: Operating Systems Fundamentals,
Spring 2023,
UiT The Arctic University of Norway
 
 ## Assignment

In this project you will write the bootup code for a simple operating system that we will be developing for the rest of the semester. The bootup code will be in real mode (instead of protected mode).

A PC can be booted up in two modes: cold boot or warm boot. Cold boot means you power down and power up the machine by pressing the power button of a PC. Warm boot means that you reset the running OS. Either way, the PC will reset the processor and run a piece of code in the BIOS to try to read the first sector from the usb drive if found, otherwise, it will try to read a sector from the hard disk drive. Once this sector is read in, the CPU will jump to the beginning of the loaded code.

Your task in this project is to implement a bootloader and the createimage program. The bootloader resides on the boot sector and its responsibility is to load the rest of the operating system image from other sectors to the memory. Createimage is a tool (in this case a Linux tool) to create a bootable operating system image on an usb disk. The image will include the bootblock and the rest of the operating system.

The bootblock and createimage from this assignment will be used throughout the semester. 

## Provided files

Detailed explanation of each file is provided in the precode. You will modify the filenames in **bold** in this assignment. 

| File 	               | Description |
|----------------------|-------------|
| Makefile             | Simplify building the collection of files into an *image*. |
| bochsrc            	 | A configuration file for bochs. |
| **bootblock.S**      | Code for the bootblock of a bootable disk. |
| bootblock_example.s  |	Some common programming tricks in assembler. Just a reference code file. |
| bootblock.given 	    | A sample bootblock binary. Use this to test your createimage implementation. |
| **createimage.c**    | Tool to create a bootable operating system image of the ELF-formated files provided to it. contains a template you can use as a strarting point when completing this tool.  |
| createimage.given    |	A working createimage binary. Use this to test your bootblock implementation. |
| kernel.s 	           | Kernel code. Contains the *\_start* initial function. Minimal kernel to be loaded by the bootup code. |

## Detailed requirements and hints for bootblock.S

This file should contain the code that will be written on the boot sector of the usb disk. This has to be written in x86 assembly and should not exceed 512 bytes (1 sector). This code will be responsible for loading in the rest of the operating system, setting up a stack for the kernel and then invoking it. You are required to handle kernels of size up to 18 sectors. However, try to make your bootblock handle sizes greater than 18 sectors.

The bootblock gets loaded at 0x07c0:0000. Your bootblock should load the OS starting at 0x0000:1000. In real mode, you have 640 KBytes starting at 0x0000:0000. The low memory area has some system data like the interrupt vectors and BIOS data. So, to avoid overwriting them, you should only use memory above 0x0000:1000 for this assignment.

To design and implement this, you need to learn about x86 architecture, CPU resets and how to read a sector from the usb drive with BIOS (described below). We have provided you with a trivial and otherwise useless kernel (**kernel.s**) to test whether your implementation works.

You can assume that the entire OS is less or equal to 18 sectors for this project. 

Useful information and documentation:
- Appendix A in Computer Organization and Design by Patterson & Hennessy describes how the linker and loader works. 
- [Intel® 64 and IA-32 Architectures Software Developer Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html). The reference for all things x86 assembler. Get all the software developers manuals (Volume 1-3). Note that Intel documents use the intel assembler syntax, whereas you use the AT&T assembler syntax.
- [A Guide to Programming Pentium/Pentium Pro Processors](pc-arch.pdf). A good introduction to get you started (or refreshed) on assembler. Covers both Intel and AT&T syntax.
- [x86 Assembly Guide](https://flint.cs.yale.edu/cs421/papers/x86-asm/asm.html). Basics of 32-bit x86 assembly language programming.

## Detailed Requirements and Hints for createimage.c

This program is a Linux tool to create a bootable image. To make your life easier, we have provided you with a [man page](createimage.md) Please ignore the "-vm" option for this project.

You should read the man pages of:
- [createimage](createimage.md) 
- od(1)
- and objdump(1)

In addition, [Executable and Linkable Format (ELF)](elfdoc.pdf) has more information about the ELF format that is used by create image.

## Design review

For the design review, you need to have figured out all the details before the actual implementation. For **bootblock.S**, you need to know where it gets loaded, how it loads the OS and what other things have to be setup before invoking the kernel. For **createimage**, you need to know what part of the executable file you need to extract and put in the "image".

Before you come to meet the TA, you should prepare for a *brief* presentation on a piece of paper or on the screen. Only oral presentation is not acceptable. 

## Handin

This project is a **mandatory assignment**. The handin is in GitHub.

You should commit:
* A report, maximum 4 pages that gives overview of how you solved each task, how you have tested your code, and any known bugs or issues.
    * The report should include your name, e-mail, and GitHub username.
    * The four page limit includes everything.
    * The report shoubd be in the doc directory in your repository.
* Your code in your private repository.

## Low-level Details

### Display memory

During booting, you can write directly to the screen by writing to the display RAM which is mapped starting at `0xb800:0000`. Each location on the screen requires two bytes—one to specify the attribute (**Use `0x07`**) and the second for the character itself. The text screen is 80x25 characters. So, to write to i-th row and j-th column, you write the 2 bytes starting at offset `((i-1)*80+(j-1))*2`.

So, the following code sequence writes the character 'K' (ascii `0x4b`) to the top left corner of the screen.

`    movw 0xb800,%bx`
`    movw %bx,%es`
`    movw $0x074b,%es:(0x0)`

This code sequence is very useful for debugging.

### BIOS Int 0x13 Function 2 (From Undocumented PC)

Reads a number of 512 bytes sectors starting from a specified location. The data read is placed into RAM at the location specified by ES:BX. The buffer must be sufficiently large to hold the data AND must not cross a 64K linear address boundary.

Called with:

    ah = 2 
    al = number of sectors to read, 1 to 36 
    ch = track number, 0 to 79 
    cl = sector number, 1 to 36 
    dh = head number, 0 or 1 
    dl = drive number, 0 to 3 
    es:bx = pointer where to place information read 

Returns:

    ah = return status (0 if successful) 
    al = number of sectors read 
    carry = 0 successful, = 1 if error occurred 

### BIOS Int 0x10 Function 0x0e (From Undocumented PC)

This function sends a character to the display at the current cursor position on the active display page. As necessary, it automatically wraps lines, scrolls and interprets some control characters for specific actions. Note : Linefeed is 0x0a and carriage return is 0x0d.

Called with:

    ah = 0x0e 
    al = character to write 
    bh = active page number (Use 0x00) 
    bl = foreground color (graphics mode only) (Use 0x02) 

Returns:

    character displayed 
