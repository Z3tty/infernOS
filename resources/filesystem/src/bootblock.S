.text
.code16

#-----------------------------------------------------------------------
# BOOTBLOCK_SEGMENT and BOOTBLOCK_POINTER:
#       Position at where the bootblock is loaded by BIOS.
#
# KERNEL_LOCATION:
#  Position at where the kernel is going to be loaded.
#
# SECTOR_SIZE:
#  Size of a USB disk sector.
#
# STACK_SEGMENT,0x7000 and STACK_POINTER,0xfffe
#  Position for the stack for bootblock.
#-----------------------------------------------------------------------

  .equ  BOOTBLOCK_SEGMENT, 0x7c0
  .equ  BOOTBLOCK_POINTER, 0x0
  .equ  KERNEL_LOCATION, KERNEL_LOAD_ADDR
  .equ  SECTOR_SIZE, 512
  .equ  STACK_SEGMENT, 0x7000
  .equ  STACK_POINTER, 0xfffe

#-----------------------------------------------------------------------
# _start
#-----------------------------------------------------------------------
.globl  _start
_start:
  jmp  over

os_size:
  # area reserved for createimage to write the OS size
  .word  0
  .word  0

over:
  # setup the stack
  movw  $STACK_SEGMENT,%ax
  movw  %ax,%ss
  movw  $STACK_POINTER,%sp

  # setup the data segments
  movw  $BOOTBLOCK_SEGMENT,%ax
  movw  %ax,%ds
  movw  $BOOTBLOCK_SEGMENT,%ax
  movw  %ax,%es

  # say hi to the user
  call  clear_screen
  pushw  $welcome_msg
  call  print_str

  # load the OS
  call  load_os

  # turn on the A20 line so we can address the entire memory
  call  set_a20

#-----------------------------------------------------------------------
# Setup the protected mode and jump to the kernel.
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# SEG_DESC_SIZE:
#  Size of segment descriptor.
#
# GDT_SEG_DESCS:
#       Number of segment descriptors in the GDT.
#
# GDT_LIMIT:
#  Index of last valid byte on the GDT.
#
# SEG_SEL_TI:
#  Segment selector table indicator flag (bit 2).  Specifies the
#  descriptor table to use: 0 for GDT, 1 for LDT.
#
# SEG_SEL_RPL_*:
#  Segment selector requested privilege levels (bits 0 and 1):
#  from 0 to 3.
#
# BOOTBLOCK_SEG_SEL:
#  Segment selector for bootblock.
#
# KERNEL_CODE_SEG_SEL and KERNEL_DATA_SEG_DEL:
#  Segment selectors for kernel.
#-----------------------------------------------------------------------

  .equ  SEG_DESC_SIZE,8
  .equ  GDT_SEG_DESCS,3
  .equ  GDT_LIMIT,GDT_SEG_DESCS * SEG_DESC_SIZE - 1
  .equ  SEG_SEL_TI,1 << 2
  .equ  SEG_SEL_RPL_0,0
  .equ  SEG_SEL_RPL_1,1
  .equ  SEG_SEL_RPL_2,2
  .equ  SEG_SEL_RPL_3,3
  .equ  NULL_SEG_SEL,		0 << 3 & ~SEG_SEL_TI | SEG_SEL_RPL_0
  .equ  BOOTBLOCK_SEG_SEL,	1 << 3 & ~SEG_SEL_TI | SEG_SEL_RPL_0
  .equ  KERNEL_CODE_SEG_SEL,	BOOTBLOCK_SEG_SEL
  .equ  KERNEL_DATA_SEG_SEL,	2 << 3 & ~SEG_SEL_TI | SEG_SEL_RPL_0

  # Make cursor invisible
  movb    $1,%ah                  # cursor type
  movw    $0x0100,%cx             # no cursor
  int     $0x10                   # BIOS call

  # Disable interrupts.
  cli

  # Load the GDTR (Global Descriptor Table Register) with the
  # base address of the GDT.

  lgdt  gdtr

  # Set the PE flag (bit 0) in control register CR0.  This
  # enables the protected mode.

  smsw  %ax
  orw  $1,%ax
  lmsw  %ax

  # Execute a far jump.  This forces CS to be loaded with the
  # proper segment selector.

  ljmp  $BOOTBLOCK_SEG_SEL,$new_world + (BOOTBLOCK_SEGMENT << 4)

.code32

new_world:

  movw  $KERNEL_DATA_SEG_SEL,%ax

  # Setup the kernel stack.
  movw  %ax,%ss
  movl  $0x80000,%esp

#-----------------------------------------------------------------------
# The _start(..) function in kernel.c expects to find the os_size
# on the stack, push it. Also simulate a call to _start(..) by pushing
# a "fake" return address onto the stack.
#-----------------------------------------------------------------------
  movzxw  os_size,  %ecx  # zero-extend os_size to a 32-bit int
  pushl   %ecx            # push 32-bit os_size value
  pushl   $0              # push a fake return address for the C _start() fn

  # Reload the segment registers.
  movw  %ax,%ds
  movw  %ax,%es

  # Jump to the kernel.
  ljmp  $KERNEL_CODE_SEG_SEL,$(KERNEL_ADDR)

.code16

#-----------------------------------------------------------------------
# void load_os(void)
#-----------------------------------------------------------------------
load_os:
  pushw  %bp
  movw  %sp,%bp
  subw  $14,%sp
  pushw  %ax
  pushw  %bx
  pushw  %cx
  pushw  %dx
  pushw  %es

  .equ  SECTOR,-2
  .equ  HEAD,-4
  .equ  TRACK,-6
  .equ  DESTSEG,-8
  .equ  NSECTORS,-10
  .equ  LAST_HEAD, -12
  .equ  LAST_SECTOR, -14

# Read drive parameters 
  movb  $8, %ah
  movb  $0x80, %dl
  int $0x13
  jc    load_os_error
  andw  $0x003f, %cx
  movw  %cx, LAST_SECTOR(%bp)
  shrw  $8,%dx
  movw  %dx,LAST_HEAD(%bp)

# Setup start parameters
  movw  $2,SECTOR(%bp)
  movw  $0,HEAD(%bp)
  movw  $0,TRACK(%bp)
  movw  $KERNEL_LOCATION >> 4,DESTSEG(%bp)
  movw  (os_size),%ax
  movw  %ax,NSECTORS(%bp)

load_os_while:
  cmpw  $0,NSECTORS(%bp)	# while nsectors > 0
  jz  load_os_end_while
  pushw  $'.'			# print a dot
  call  print_char
  movw  LAST_SECTOR(%bp), %ax
  cmpw  %ax,SECTOR(%bp)		# if last sector
  jle  read_sector
  movw  $1,SECTOR(%bp)		# then, sector = 1, and update head
  incw  HEAD(%bp)
  movw  LAST_HEAD(%bp), %ax # if last head 
  cmpw  %ax, HEAD(%bp)
  jle read_sector
  movw  $0, HEAD(%bp)     # then, head = 0, and update cylinder
  incw  TRACK(%bp)

read_sector:  
  movb  TRACK(%bp),%ch  	# track number
  movb  SECTOR(%bp),%cl		# sector number
  movb  HEAD(%bp),%dh		  # head number
  movb  $0x80,%dl		      # hard disk drive number
  movw  DESTSEG(%bp),%ax	# destination in es:bx
  movw  %ax,%es
  movw  $0x0,%bx
  movb  $2,%ah			# function number
  movb  $1,%al			# number of sectors to read
  int  $0x13

  cmpb  $0,%ah			# check if read was successful
  jne  load_os_error
  incw  SECTOR(%bp)		# prepare to read next sector
  addw  $SECTOR_SIZE >> 4,DESTSEG(%bp)
  decw  NSECTORS(%bp)
  jmp  load_os_while
load_os_end_while:

  pushw  $load_os_done_msg
  call  print_str

# wait for about 2 seconds
  movw  $5,%cx
load_os_wait:
  pushw  $0x8236
  call  delay
  loopnz  load_os_wait

  popw  %es
  popw  %dx
  popw  %cx
  popw  %bx
  popw  %ax
  movw  %bp,%sp
  popw  %bp
  retw

load_os_error:
  pushw  $load_os_error_msg
  call  print_str
  jmp  forever

#-----------------------------------------------------------------------
# void set_a20(void)
#-----------------------------------------------------------------------
set_a20:
  pushw  %bp
  movw  %sp,%bp
  pushw  %ax

  # write output port
set_a20_loop1:
  in  $0x64,%al
  test  $2,%al
  loopnz  set_a20_loop1
  movb  $0xd1,%al
  out  %al,$0x64

  # enable A20 address line
set_a20_loop2:
  in  $0x64,%al
  test  $2,%al
  loopnz  set_a20_loop2
  movb  $0xdf,%al
  out  %al,$0x60

set_a20_loop3:
  in  $0x64,%al
  test  $2,%al
  loopnz  set_a20_loop3

  popw  %ax
  movw  %bp,%sp
  popw  %bp
  retw

#-----------------------------------------------------------------------
# void delay(short count)
#
# Provides small-value timing delays.  The count is in 15us increments.
# The timing for the delay is based on the system's quartz crystal
# oscillator, which drives the processor clock.  This forms an
# instruction loop checking the toggled value of an I/O port.
# (Adapted from Burgess95.)
#-----------------------------------------------------------------------
delay:
  pushw  %bp
  movw  %sp,%bp
  pushw  %ax
  pushw  %cx

  movw  4(%bp),%cx	# get delay count
  cmpw  $0,%cx		# return if it is zero
  je  delay_01

delay_00:
  in  $0x61,%al	# get system status port
  andb  $0x10,%al	# check refresh bit
  cmpb  %al,%ah		# check toggle of bit
  je  delay_00	# no toggle yet
  movb  %al,%ah		# toggle! move to %ah for next compare
  loop  delay_00

delay_01:
  popw  %cx
  popw  %ax
  movw  %bp,%sp
  popw  %bp
  retw  $2

#-----------------------------------------------------------------------
# void print_char(short c)
#-----------------------------------------------------------------------
print_char:
  pushw  %bp
  movw  %sp,%bp
  pushw  %ax
  pushw  %bx

  movb  $0x0e,%ah	# function number
  movb  4(%bp),%al	# character to write
  movb  $0x00,%bh	# active page number
  movb  $0x02,%bl	# foreground color
  int  $0x10

  popw  %bx
  popw  %ax
  movw  %bp,%sp
  popw  %bp
  retw  $2

#-----------------------------------------------------------------------
# void print_str(char *s)
#-----------------------------------------------------------------------
print_str:
  pushw  %bp
  movw  %sp,%bp
  pushw  %ax
  pushw  %bx
  pushw  %si

  movw  4(%bp),%si
print_str_while:
  lodsb  # load character to write into al, and increment si
  cmpb  $0,%al
  jz  print_str_end_while
  movb  $0x0e,%ah	# function number
  movb  $0x00,%bh	# active page number
  movb  $0x02,%bl	# foreground color
  int  $0x10
  jmp  print_str_while
print_str_end_while:

  popw  %si
  popw  %bx
  popw  %ax
  movw  %bp,%sp
  popw  %bp
  retw  $2

#-----------------------------------------------------------------------
# void clear_screen(void)
#-----------------------------------------------------------------------
clear_screen:
  pushw  %ax
  movb  $0,%ah		# function number
  movb  $3,%al		# video mode
  int  $0x10
  popw  %ax
  retw

#-----------------------------------------------------------------------
# strings
#-----------------------------------------------------------------------

welcome_msg:
  .asciz  "OS P6\n\rLoading"

load_os_done_msg:
  .asciz  "\n\rDone"

load_os_error_msg:
  .asciz  "\n\rError while reading from USB"

#-----------------------------------------------------------------------
# infinite loop
#-----------------------------------------------------------------------
forever:
  jmp  forever

#-----------------------------------------------------------------------
# The GDTR (Global Descriptor Table Register) holds the 32-bit base
# address and the 16-bit table limit for the GDT.  The base address
# specifies the linear address of byte 0 of the GDT; the table limit
# specifies the number of bytes in the table.
#
# The limit value is added to the base address to get the address of the
# last valid byte.  A limit value of 0 results in exactly one valid
# byte.  Because segments descriptors are always 8 bytes long, the GDT
# limit should always be one less than an integral multiple of eight
# (that is, 8N - 1). (Intel Vol 3, p. 3-16)
#-----------------------------------------------------------------------

gdtr:
  .word  GDT_LIMIT			# limit
  .long  gdt + (BOOTBLOCK_SEGMENT << 4)	# base address

#-----------------------------------------------------------------------
# The base address of the GDT (Global Descriptor Table) should be
# aligned on an eight-byte boundary to yield the best processor
# performance. (Intel Vol 3, p. 3-16)
#-----------------------------------------------------------------------

.align 8

gdt:
  # the first entry of the GDT is not used by the processor
  .long  0
  .long  0

  # kernel code segment descriptor (same for bootblock)
  .word  0xffff	      # limit
  .word  0x0000	      # base 15:00
  .byte  0x00	      # base 23:16
  .word  0xc09a	      # flags
  .byte  0x00	      # base 31:24

  # kernel data segment descriptor
  .word  0xffff	      # limit
  .word  0x0000	      # base 15:00
  .byte  0x00	      # base 23:16
  .word  0xc092	      # flags
  .byte  0x00	      # base 31:24

. = 510
  .byte 0x55, 0xaa
