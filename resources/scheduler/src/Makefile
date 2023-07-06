# Makefile for OS project 2.

FLOPPYDEV ?= $(shell ./flpdevdetect)

# Compiler and linker used

CC = gcc
LD = ld

CREATEIMAGE=./createimage

# Address of the kernel, process 1 and process 2
KERNEL_ADDR		= 0x1000
PROC1_ADDR		= 0x5000
PROC2_ADDR		= 0x7000

# Compiler flags
#
# -fno-builtin:
# Don't recognize builtin functions that do not begin with
# '__builtin_' as prefix.
#
# -fno-defer-pop:
# Always pop the arguments to each function call as soon as that
# function returns.
#
# -fomit-frame-pointer:	
# Don't keep the frame pointer in a register for functions that don't
# need one.
#
# -O2:
# Turn on all optional optimizations except for loop unrolling and
# function inlining.
#
# -c:
# Compile or assemble the source files, but do not link.
#
# -Wall:
# Enable all warnings (Same as all `-W' options combined)
#
# -DKERNEL_ADDR=$(KERNEL_ADDR), -DPROC1_ADDR, etc:
# Define macro KERNEL_ADDR as $(KERNEL_ADDR). The macro is used by the
# C preprocessor.

# Newer compilers reorder functions and variables compared to the
# source file. This breaks the make process since we depend on 
# a known symbol (_start for the kernel) to be first in the object
# file. -fno-toplevel-reorder makes sure that the newer compiler works 
# the same way as older compilers. 
# 
# -fno-stack-protector fixes a missing symbol problem when linking. 
# We have not added any support for stack protection in the kernel, 
# so we remove it (again, this is restoring old behaviour).

CCOPTS = -Wall -Wextra -c -fomit-frame-pointer -O2 -fno-builtin -fno-defer-pop \
         -fno-stack-protector -fno-toplevel-reorder -fno-unit-at-a-time -m32 \
         -mfpmath=387 -march=i386 -mno-mmx -mno-sse -mno-sse2 -fno-asynchronous-unwind-tables \
         -DKERNEL_ADDR=$(KERNEL_ADDR)


# Linker flags
#
# --nmagic
#  Barebones linking: do not use system libraries, do not page-align sections,
#  just lump the .text and .data sections together into one loadable chunk.
#
# -nostdlib:
# Don't use the standard system libraries and startup files when
# linking. Only the files you specify will be passed to the linker.
#          
# -Ttext <xxxx>:
# Use <xxxx> as the starting address for the text segment of the output
# file.

LDOPTS = --nmagic -nostdlib -melf_i386

# Common objects used by both the kernel and user processes
COMMON = util.o

# Objects needed by the processes we'll create. Note that the order in
# which the objects appear, is extremely important -- the object with
# the process' entry point needs to come first.
PROCESS1_OBJS = process1.o syslib.o $(COMMON)
PROCESS2_OBJS = process2.o syslib.o $(COMMON)

# Objects needed by the kernel
KERNEL_OBJS = kernel.o entry.o scheduler.o lock.o th1.o th2.o th3.o $(COMMON) 

# Makefile targets
all: image

kernel: $(KERNEL_OBJS)
	$(LD) $(LDOPTS) -Ttext $(KERNEL_ADDR) -o kernel $^

# This is a ugly hack beacuse the compiler doesn't put the symbol '_start'
# first any more, so we need to compile the processes and exctract the
# start address before compiling kernel.c
# update, 2009: need to grep for "T _start"  and not start since there is a __bss_start in there as well 
kernel.o: kernel.c process1 process2
	$(CC) $(CCOPTS) -DPROC1_ADDR=0x$(shell nm process1 | grep "T _start" | cut -c-8) -DPROC2_ADDR=0x$(shell nm process2 | grep "T _start" | cut -c-8) -o kernel.o kernel.c

entry.o: entry.S
	$(CC) $(CCOPTS) -E $< > entry-pp.s
	$(CC) $(CCOPTS) -c entry-pp.s -o entry.o

process1: $(PROCESS1_OBJS)
	$(LD) $(LDOPTS) -Ttext $(PROC1_ADDR) -o process1 $^

process2: $(PROCESS2_OBJS)
	$(LD) $(LDOPTS) -Ttext $(PROC2_ADDR) -o process2 $^

bootblock: bootblock.o
	$(LD) $(LDOPTS) -Ttext 0x0 -o bootblock $<

createimage: createimage.o
	$(CC) -m32 -o createimage $<

# Create an image to put on the floppy
image: $(CREATEIMAGE) bootblock kernel process1 process2
	strip --remove-section=.note.gnu.property $^
	$(CREATEIMAGE) bootblock kernel process1 process2

# Put the image on the floppy (these two stages are independent, as
# both vmware and bochs can run using only the image file stored on
# the harddisk)

boot: image
ifneq ($(FLOPPYDEV),)
	dd if=./image of=$(FLOPPYDEV) bs=512
else
	@echo -e "Cannot seem to find a usable floppy drive. If you think you\
have a usable\ndevice that isn't detected, try this:\
'make FLOPPYDEV=/dev/somdevice boot'"
endif

# Figure out dependencies, and store them in the hidden file .depend
depend: .depend
.depend:
	$(CC) $(CCOPTS) -MM *.c > $@

# Clean up!
clean:
	rm -f *.o
	rm -f bootblock createimage image kernel process1 process2
	rm -f .depend
	rm -f entry-pp.s

# No, really, clean up!
distclean: clean
	rm -f *~
	rm -f \#*
	rm -f *.bak
	rm -f bochsout.txt

# How to compile a C file
%.o:%.c
	$(CC) $(CCOPTS) $<

# How to assemble
%.o:%.s
	$(CC) $(CCOPTS) $<

%.o:%.S
	$(CC) $(CCOPTS) -x assembler-with-cpp $<

# How to produce assembler input from a C file
%.s:%.c
	$(CC) $(CCOPTS) -S $<

# Include dependencies (the leading dash prevents warnings if the file
# doesn't exist)
-include .depend
