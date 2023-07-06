# Makefile for the OS projects.

FLOPPYDEV ?= $(shell ./flpdevdetect)

CC = gcc -m32
LD = ld

# CREATEIMAGE=./createimage.given
# BOOTBLOCK=bootblock.given

# When you are ready to test your own implementation of createimage, you
# should uncomment the following line:
#CREATEIMAGE=./createimage

# When you are ready to test your own implementation of the bootblock, you
# should uncomment the following line:
BOOTBLOCK=bootblock


# Where to locate the kernel in memory
KERNEL_ADDR = 0x1000

# Compiler flags
#
# -fno-builtin: 
# Don't recognize builtin functions that do not begin with
# '__builtin_' as prefix.
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
# -m32:
# Compile for 32-bit CPUs
CCOPTS = -Wall -Wextra -g -c -fomit-frame-pointer -O2 -fno-builtin -m32 -DKERNEL_ADDR=$(KERNEL_ADDR)

# Linker flags
#
# -n:
# Turn off page alignment for sections, because we are setting the alignment.
#
# -nostdlib:
# Don't use the standard system libraries and startup files when
# linking. Only the files you specify will be passed to the linker.
#          
# -melf-i386:
# Emulate the i386 ELF linker
#
# -Ttext <xxxx>:
# Use <xxxx> as the starting address for the text segment of the
# output file.

LDOPTS = -n -nostdlib -melf_i386 -Ttext

# Makefile targets
all: image

kernel: kernel.o
	$(LD) $(LDOPTS) $(KERNEL_ADDR) -o kernel $<

bootblock: bootblock.o
	$(LD) $(LDOPTS) 0x0 -o bootblock $<

createimage: createimage.o
	$(CC) -m32 -o createimage $<

# Create an image to put on the floppy
image: $(BOOTBLOCK) kernel $(CREATEIMAGE)
	$(CREATEIMAGE) --extended $(BOOTBLOCK) kernel


# Put the image on the floppy (these two stages are independent, as both
# vmware and bochs can run using an image file stored on the harddisk)	
boot: image
ifneq ($(FLOPPYDEV),)
	dd if=./image of=$(FLOPPYDEV) bs=512
else
	@echo -e "Cannot seem to find a usable floppy drive. If you think you\
have a usable\ndevice that isn't detected, try this:\
'make FLOPPYDEV=/dev/somdevice boot'"
endif

# Clean up!
clean:
	rm -f *.o
	rm -f createimage image bootblock kernel

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

