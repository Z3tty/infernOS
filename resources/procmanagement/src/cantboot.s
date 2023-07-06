.text
.code16

#-----------------------------------------------------------------------
# This is the "boot block" for data disk that won't boot.
#-----------------------------------------------------------------------

  .equ  BOOTBLOCK_SEGMENT,0x7c0
  .equ  BOOTBLOCK_POINTER,0x0
  .equ  KERNEL_LOCATION,0x8000
  .equ  SECTOR_SIZE,512
  .equ  STACK_SEGMENT,0x7000
  .equ  STACK_POINTER,0xfffe

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

  # crash and burn
  jmp  	forever

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
  .asciz  "D-240 OS program disk.\n\rCan not boot using this disk!"


#-----------------------------------------------------------------------
# infinite loop
#-----------------------------------------------------------------------
forever:
  jmp  forever





