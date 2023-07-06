# Makefile for the OS projects.

CC = gcc
LD = ld

CREATEIMAGE=./createimage

# Address of the kernel, process 1 and process 2
KERNEL_ADDR	= 0x1000
PROC1_ADDR	= 0x10000
PROC2_ADDR	= 0x12000

CCOPTS = -Wall -Wextra -c -O2 -fno-builtin \
		 -DKERNEL_ADDR=$(KERNEL_ADDR) \
		 -DPROC1_ADDR=$(PROC1_ADDR) \
		 -DPROC2_ADDR=$(PROC2_ADDR)

# The following flags are added to allow the code to compile on newer
# versions of gcc (gcc 4.X+). If your compiler complaints about not
# knowing these flags, its probably safe to remove them.
# 
# Newer compilers reorder functions and variables compared to the
# source file. This breaks the make process since we depend on 
# a known symbol (_start for the kernel) to be first in the object
# file. -fno-toplevel-reorder makes sure that the newer compiler works 
# the same way as older compilers. 
# 
# -fno-stack-protector fixes a missing symbol problem when linking. 
# We have not added any support for stack protection in the kernel, 
# so we remove it (again, this is restoring old behaviour). 
CCOPTS += -fno-stack-protector -fno-toplevel-reorder -fno-unit-at-a-time -m32
CCOPTS += -mfpmath=387 -march=i386 -mno-mmx -mno-sse -mno-sse2 -fno-asynchronous-unwind-tables

# Linker flags
LDOPTS = -N -nostdlib -melf_i386

# Common objects used by both the kernel and user processes
COMMON		= util.o

# Objects needed by the processes we'll create. Note that the order in
# which the objects appear, is extremely important -- the object with
# the process' entry point needs to come first and the entry point
# must be the first code in the object file
PROCESS1_OBJS	= process1.o syslib.o $(COMMON)
PROCESS2_OBJS	= process2.o syslib.o $(COMMON)

# Objects needed by the kernel. The object file with the kernel entry
# point must be the first in this list and the entry point must be the
# first code in the object file.
KERNEL_OBJS = kernel.o entry.o scheduler.o dispatch.o interrupt.o \
th1.o th2.o th3.o thread.o time.o philosophers.o barrier_test.o $(COMMON)


# Makefile targets
.PHONY: all boot depend clean distclean
all: image

kernel: $(KERNEL_OBJS)
	$(LD) $(LDOPTS) -Ttext $(KERNEL_ADDR) -o $@ $^

entry.o: entry.S asmsyms.h
	$(CC) -m32 -c -x assembler-with-cpp -o $@ $<

dispatch.o : dispatch.S asmsyms.h pcb_status.h
	$(CC) -m32 -c -x assembler-with-cpp -o $@ $<

asmsyms.h: asmdefs
	./asmdefs > $@

asmdefs: asmdefs.c kernel.h
	$(CC) -m32 -o $@ $<

process1: $(PROCESS1_OBJS)
	$(LD) $(LDOPTS) -Ttext $(PROC1_ADDR) -o $@ $^

process2: $(PROCESS2_OBJS)
	$(LD) $(LDOPTS) -Ttext $(PROC2_ADDR) -o $@ $^

bootblock: bootblock.o
	$(LD) $(LDOPTS) -Ttext 0x0 -o $@ $<

createimage: createimage.o
	$(CC) -m32 -o $@ $<

# Create an image to put on the floppy
image: $(CREATEIMAGE) bootblock kernel process1 process2
	strip --remove-section=.note.gnu.property $^
	$(CREATEIMAGE) bootblock kernel process1 process2

# Put the image on the floppy (these two stages are independent, as
# both vmware and bochs can run using only the image file stored on
# the harddisk)
boot: image
ifneq ($(FLOPPYDEV),)
	dd if=image of=$(FLOPPYDEV) bs=512
else
	@echo -e "Cannot seem to find a usable floppy drive. If you think you\
have a usable\ndevice that isn't detected, try this:\
'make FLOPPYDEV=/dev/somedevice boot'"
endif

# Figure out dependencies, and store them in the hidden file .depend
depend: .depend

.depend: *.[chS]
	$(CC) $(CCOPTS) -MM -MG *.[cS] > $@

# Clean up!
clean:
	-$(RM) *.o
	-$(RM) asmsyms.h asmdefs
	-$(RM) bootblock createimage image kernel process1 process2
	-$(RM) .depend

# No, really, clean up!
distclean: clean
	-$(RM) *~
	-$(RM) \#*
	-$(RM) *.bak
	-$(RM) bochsout.txt

# How to compile a C file
%.o:%.c
	$(CC) $(CCOPTS) $<

# How to assemble
%.o:%.s
	$(CC) $(CCOPTS) $<

%.o:%.S
	$(CC) $(CCOPTS) -x assembler-with-cpp $<

# Include dependencies (the leading dash prevents warnings if the file
# doesn't exist)
-include .depend
