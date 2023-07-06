# Makefile for the OS projects.

CC = gcc
LD = ld

KERNEL_LOCATION    = 0x8000 # physical & virtual address of kernel
PROCESS_LOCATION   = 0x1000000 # virtual address of processes

# Compiler flags
CCOPTS = -Wall -Wextra -Wno-unused -g -c -m32 -O2 -fno-builtin -fno-unit-at-a-time -fno-stack-protector -fno-toplevel-reorder -fno-defer-pop \
         -mfpmath=387 -march=i386 -mno-mmx -mno-sse -mno-sse2 \
         -DPROCESS_START=$(PROCESS_LOCATION)

# Linker flags
LDOPTS = -znorelro -nostdlib -melf_i386 --nmagic
# Add your user program here:
USER_PROGRAMS =	

KERNEL = kernel.o

# Common objects used by both the kernel and user processes
COMMON = util.o print.o
# Processes to create
PROCESSES = shell.o process1.o process2.o process3.o process4.o

# USB subsystem
USB = usb/pci.o usb/uhci_pci.o usb/uhci.o usb/ehci_pci.o usb/usb_hub.o \
			usb/usb.o usb/usb_msd.o usb/scsi.o usb/usb_hid.o usb/usb_keyboard.o \
			usb/allocator.o 

# Objects needed by the kernel
KERNELOBJ = entry.o $(COMMON) th1.o th2.o thread.o scheduler.o interrupt.o \
		mbox.o keyboard.o memory.o sleep.o time.o \
		dispatch.o tlb.o $(USB)

# Objects needed to build a process
PROCOBJ = $(COMMON) syslib.o


# Targets that aren't files (phony targets)
.PHONY: all demo progdisk clean distclean

### Makefile targets

all: bootblock createimage kernel image $(PROCESSES:.o=)

kernel: $(KERNELOBJ) $(KERNEL)
	$(LD) $(LDOPTS) -Ttext $(KERNEL_LOCATION) -o $@ $^
	objcopy $@ $@ -G kernel_start

entry.o: entry.S
	$(CC) $(CCOPTS) -x assembler-with-cpp -c $< -o $@

# The processes
process1: proc_start.o process1.o $(PROCOBJ)
	$(LD) $(LDOPTS) -Ttext $(PROCESS_LOCATION) -o $@ $^

process2: proc_start.o process2.o $(PROCOBJ)
	$(LD) $(LDOPTS) -Ttext $(PROCESS_LOCATION) -o $@ $^

process3: proc_start.o process3.o $(PROCOBJ)
	$(LD) $(LDOPTS) -Ttext $(PROCESS_LOCATION) -o $@ $^

process4: proc_start.o process4.o $(PROCOBJ)
	$(LD) $(LDOPTS) -Ttext $(PROCESS_LOCATION) -o $@ $^

shell: proc_start.o shell.o $(PROCOBJ)
	$(LD) $(LDOPTS) -Ttext $(PROCESS_LOCATION) -o $@ $^


# other stuff
createimage: createimage.c
	$(CC) -o $@ $<  

asmsyms.h: asmdefs
	./$< > $@

asmdefs: asmdefs.c kernel.h
	$(CC) -o $@ $<

bootblock.o: bootblock.S kernel
	$(CC) $(CCOPTS) -DKERNEL_LOAD_ADDR=$(KERNEL_LOCATION) -DKERNEL_ADDR=0x$(shell nm kernel | grep "T kernel_start" | cut -c-8) -x assembler-with-cpp -o $@ $<

bootblock: bootblock.o kernel
	$(LD) $(LDOPTS) -Ttext 0x0 -o $@ $<

# Create an image to put on the USB
image: createimage bootblock kernel $(PROCESSES:.o=)
	strip --remove-section=.note.gnu.property $^
	./createimage --vm --kernel ./bootblock ./kernel $(PROCESSES:.o=)

# Clean up!
clean:
	-$(RM) *.o
	-$(RM) usb/*.o
	-$(RM) asmsyms.h
	-$(RM) $(PROCESSES:.o=) kernel image createimage bootblock asmdefs
	-$(RM) .depend

# No, really, clean up!
distclean: clean
	-$(RM) *~
	-$(RM) \#*
	-$(RM) *.bak
	-$(RM) TAGS tags
	-$(RM) bochsout.txt
	-$(RM) serial.out

# Make tags for emacs
TAGS:   *.[chsS]
	$(ETAGS) $^

# Make tags for vi
tags:   *.[chsS]
	$(TAGS) $^

# How to compile a C file
%.o:%.c
	$(CC) $(CCOPTS) -o $@ $<

# How to assemble
%.o:%.s
	$(CC) $(CCOPTS) $<

%.o:%.S
	$(CC) $(CCOPTS) -x assembler-with-cpp $< 

# How to produce assembler input from a C file
%.s:%.c
	$(CC) $(CCOPTS) -S $<

# Generate dependency metadata from source files
# ----------------------------------------------------------------------
# We have GCC generate dependency metadata and store it in Makefile format
# in a hidden file: .depend

# Dependency generation rule
#
# -MM	turn on dependency generation mode
# -MG	treat missing headers as more depedencies to be generated, not errors
# -MF >(cat > $@)	safer redirection: pipe deps (and only deps) to file
#
.depend: $(wildcard *.[cS])
	$(CC) $(CCOPTS) -MM -MG -MF >(cat > $@) $^

# Use bash as the shell for this rule.
# We need bash for the '-MF' safer redirection.
.depend: SHELL:=/bin/bash

# Include generated depedencies (make them if necessary)
include .depend
