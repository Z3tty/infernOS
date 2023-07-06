.text                                                                   # Code segment
.code16                                                                 # Real Mode

  .equ  BOOTBLOCK_SEGMENT, 0x7c0                                        # Position of boot loaded by bios
  .equ  BOOTBLOCK_POINTER, 0x0                                          # ^^^
  .equ  KERNEL_LOCATION,   KERNEL_LOAD_ADDR                             # Where to load the kernel
  .equ  SECTOR_SIZE,       512                                          # Size of a USB disk sector
  .equ  STACK_SEGMENT,     0x7000                                       # Stack position
  .equ  STACK_POINTER,     0xfffe                                       # ^^^

.globl  _start                                                          # Globally declared starting point
_start:
  jmp  over                                                             # Skip over globals in memory

os_size:                                                                # Reserved for imager to write size
  .word  0
  .word  0

over:
  movw  $STACK_SEGMENT, %ax                                             # Set up stack
  movw  %ax, %ss
  movw  $STACK_POINTER, %sp

  movw  $BOOTBLOCK_SEGMENT, %ax                                         # Set up data segments
  movw  %ax, %ds
  movw  $BOOTBLOCK_SEGMENT, %ax
  movw  %ax, %es

  call  clear_screen
  pushw  $welcome_msg                                                   # Startup message
  call  print_str

  call  load_os                                                         # Load the OS and turn on A20
  call  set_a20

                                                                        # Setup protected mode and jump to the kernel
  .equ  SEG_DESC_SIZE,          8                                       # Size of segment descriptor
  .equ  GDT_SEG_DESCS,          3                                       # Number of descriptors in GDT
  .equ  GDT_LIMIT,              GDT_SEG_DESCS * SEG_DESC_SIZE - 1       # Idx of last valid GDT byte
  .equ  SEG_SEL_TI,             1 << 2                                  # Seg selector table indicator flag (0 GDT/1 LDT)
  .equ  SEG_SEL_RPL_0,          0                                       # Seg selector privilege levels 0
  .equ  SEG_SEL_RPL_1,          1                                       # 1
  .equ  SEG_SEL_RPL_2,          2                                       # 2
  .equ  SEG_SEL_RPL_3,          3                                       # 3
  .equ  NULL_SEG_SEL,		    0 << 3 & ~SEG_SEL_TI | SEG_SEL_RPL_0    # NULL selector
  .equ  BOOTBLOCK_SEG_SEL,	    1 << 3 & ~SEG_SEL_TI | SEG_SEL_RPL_0    # Bootblock selector
  .equ  KERNEL_CODE_SEG_SEL,	BOOTBLOCK_SEG_SEL                       # Selectors for the kernel
  .equ  KERNEL_DATA_SEG_SEL,	2 << 3 & ~SEG_SEL_TI | SEG_SEL_RPL_0    # ^^^

                                                                        # Make cursor invisible
  movb    $1,%ah                                                        # cursor type
  movw    $0x0100,%cx                                                   # no cursor
  int     $0x10                                                         # BIOS call
  cli                                                                   # Disable interrupts
  lgdt  gdtr                                                            # Load GDTR (GDT Register) with base GDT addr
                                                                        # Set the PE flag in control register CR0 (enable protected)
  smsw  %ax
  orw  $1,%ax
  lmsw  %ax
  ljmp  $BOOTBLOCK_SEG_SEL,$new_world + (BOOTBLOCK_SEGMENT << 4)        # Force CS to load with proper seg select by farjump

.code32                                                                 # Protected Mode
new_world:
  movw  $KERNEL_DATA_SEG_SEL,%ax
                                                                        # Setup kernel stack
  movw  %ax,%ss
  movl  $0x80000,%esp
                                                                        # OS expects os_size on stack so we push it
                                                                        # simulates _start(...) call by pushing fake ret addr
  movzxw  os_size,  %ecx                                                # zero-extend os_size to i32
  pushl   %ecx                                                          # push 32-bit os_size
  pushl   $0                                                            # push fake ret addr for _start(...)
  movw  %ax,%ds                                                         # Reload seg registers
  movw  %ax,%es
  ljmp  $KERNEL_CODE_SEG_SEL,$(KERNEL_ADDR)                             # Jump to kernel

.code16                                                                 # Real Mode
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
  cmpw  $0,NSECTORS(%bp)	                                            # while nsectors > 0
  jz  load_os_end_while
  pushw  $'.'			                                                # print a dot
  call  print_char
  movw  LAST_SECTOR(%bp), %ax
  cmpw  %ax,SECTOR(%bp)		                                            # if last sector
  jle  read_sector
  movw  $1,SECTOR(%bp)		                                            # then, sector = 1, and update head
  incw  HEAD(%bp)
  movw  LAST_HEAD(%bp), %ax                                             # if last head 
  cmpw  %ax, HEAD(%bp)
  jle read_sector
  movw  $0, HEAD(%bp)                                                   # then, head = 0, and update cylinder
  incw  TRACK(%bp)

read_sector:  
  movb  TRACK(%bp),%ch  	                                            # track number
  movb  SECTOR(%bp),%cl		                                            # sector number
  movb  HEAD(%bp),%dh		                                            # head number
  movb  $0x80,%dl		                                                # hard disk drive number
  movw  DESTSEG(%bp),%ax	                                            # destination in es:bx
  movw  %ax,%es
  movw  $0x0,%bx
  movb  $2,%ah			                                                # function number
  movb  $1,%al			                                                # number of sectors to read
  int  $0x13
  cmpb  $0,%ah			                                                # check if read was successful
  jne  load_os_error
  incw  SECTOR(%bp)		                                                # prepare to read next sector
  addw  $SECTOR_SIZE >> 4,DESTSEG(%bp)
  decw  NSECTORS(%bp)
  jmp  load_os_while
load_os_end_while:
  pushw  $load_os_done_msg
  call  print_str
                                                                        # wait for about 2s
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

                                                                        # IN: short count;
delay:                                                                  # Short delays, count in 15us incs, based on system quartz
  pushw  %bp
  movw  %sp,%bp
  pushw  %ax
  pushw  %cx
  movw  4(%bp),%cx	                                                    # get delay count
  cmpw  $0,%cx		                                                    # return if it is zero
  je  delay_01

delay_00:
  in  $0x61,%al	                                                        # get system status port
  andb  $0x10,%al	                                                    # check refresh bit
  cmpb  %al,%ah		                                                    # check toggle of bit
  je  delay_00	                                                        # no toggle yet
  movb  %al,%ah		                                                    # toggle! move to %ah for next compare
  loop  delay_00

delay_01:
  popw  %cx
  popw  %ax
  movw  %bp,%sp
  popw  %bp
  retw  $2

                                                                        # IN: short c;
print_char:
  pushw  %bp
  movw  %sp,%bp
  pushw  %ax
  pushw  %bx
  movb  $0x0e,%ah	                                                    # function number
  movb  4(%bp),%al	                                                    # character to write
  movb  $0x00,%bh	                                                    # active page number
  movb  $0x02,%bl	                                                    # foreground color
  int  $0x10
  popw  %bx
  popw  %ax
  movw  %bp,%sp
  popw  %bp
  retw  $2

                                                                        # IN: char *s;
print_str:
  pushw  %bp
  movw  %sp,%bp
  pushw  %ax
  pushw  %bx
  pushw  %si
  movw  4(%bp),%si

print_str_while:
  lodsb                                                                 # load character to write into al, and increment si
  cmpb  $0,%al
  jz  print_str_end_while
  movb  $0x0e,%ah	                                                    # function number
  movb  $0x00,%bh	                                                    # active page number
  movb  $0x02,%bl	                                                    # foreground color
  int  $0x10
  jmp  print_str_while
print_str_end_while:

  popw  %si
  popw  %bx
  popw  %ax
  movw  %bp,%sp
  popw  %bp
  retw  $2

clear_screen:
  pushw  %ax
  movb  $0,%ah		                                                    # function number
  movb  $3,%al		                                                    # video mode
  int  $0x10
  popw  %ax
  retw

                                                                        # ascii's

welcome_msg:
  .asciz  "infernOS ALPHA\n\rLoading"
load_os_done_msg:
  .asciz  "\n\rinfernOS loaded."
load_os_error_msg:
  .asciz  "\n\rError while reading from USB"

                                                                        # HANG
forever:
  jmp  forever
# GDTR holds the 32b base address and 16b limit for the GDT
# The base address specifies the linear addr of byte 0 of the GDT
# the table limit specifies the number of bytes in the table
# limit is added to base to get addr of last valid byte
# limit of 0 means ONE valid byte
# seg descriptors are always 8 bytes long, so GDT
# limit should ALWAYS be one less than 8N - 1 (Intel Vol 3, p. 3-16)
gdtr:
  .word  GDT_LIMIT			                                            # limit
  .long  gdt + (BOOTBLOCK_SEGMENT << 4)	                                # base address
# The base of the GDT should be aligned on an 8b boundary to yield the best processor perf. (Intel Vol 3, p. 3-16)
.align 8

gdt:
                                                                        # first entry is not used by processor
  .long  0
  .long  0
                                                                        # kernel code segment descriptor
  .word  0xffff	                                                        # limit
  .word  0x0000	                                                        # base 15:00
  .byte  0x00	                                                        # base 23:16
  .word  0xc09a	                                                        # flags
  .byte  0x00	                                                        # base 31:24
                                                                        # kernel data segment descriptor
  .word  0xffff	                                                        # limit
  .word  0x0000	                                                        # base 15:00
  .byte  0x00	                                                        # base 23:16
  .word  0xc092	                                                        # flags
  .byte  0x00	                                                        # base 31:24
. = 510
  .byte 0x55, 0xaa                                                      # Magic voodoo