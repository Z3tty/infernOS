# bootblock.s

# .equ symbol, expression
# These directive set the value of the symbol to the expression
  .equ    BOOT_SEGMENT,      0x07c0
  .equ    DISPLAY_SEGMENT,   0xb800
  .equ    KERNEL_SEGMENT,    0x0000
  .equ    KERNEL_OFFSET,     0x0100
  # --
  .equ    DATA_SELECTOR,     0x10
  .equ    CODE_SELECTOR,     0x08
  .equ    SECTORS,           18 # Lord forgive me
  .equ    SSIZE,             0x2000
# You need to decide where to put the stack
  .equ    STACK_SEGMENT,     0x08000 # Allocate FFFFh for the stack
  .equ    STACK_POINTER,     0x09f00 # Place the stack high in memory so that it can grow down without too much issue, without overlapping VRAM/VROM

.text                   # Code segment
.globl    _start        # The entry point must be global
.code16                 # Real mode
.org 0x0
#
# The first instruction to execute in a program is called the entry
# point. The linker expects to find the entry point in the "symbol" _start
# (with underscore).
#

_start:
  jmp beyondReservedSpace

kernelSize:
  .word 0               # bootimage will write size of kernel, in sectors

drive:
 .byte 0

sectors_per_track:
 .byte 0
heads_per_cyl:
 .byte 0

beyondReservedSpace:
  cli                   # Disable interrupts during setup
  # Load GDT
  # lgdt gdt_desc
  # Set up segment registers to the boot segment
  movw $STACK_SEGMENT, %ax
  movw %ax, %ss
  movw $STACK_POINTER, %sp
  movw $BOOT_SEGMENT, %ax
  movw %ax, %ds
  ljmp $BOOT_SEGMENT, $after_setup
after_setup:
  sti # We're about to use hardware interrupts, must be enabled
  # Store drive
  movb %dl, drive
  # Read disk params
  movw $0x0800, %ax
  # 0-out
  xorw %bx, %bx
  movw %bx, %es
  xorw %di, %di
  int $0x13 # Int13, AH = 8h
  movw %cx, %ax
  andw $0x3f, %ax # Get the lower 6 bits of cx
  movb %al, sectors_per_track
  movb %dh, %al
  incb %al # God forbid that we shouldnt be beholden to standards from the 90's, anyway head is always off-by-1
  movb %al, heads_per_cyl
copy:
  movl $SECTORS, %edi # Should be variable based on kernelSize but I am stupid and cant get it to work :)
  movw $KERNEL_OFFSET, %ax # Place kernel at 1000h
  movw %ax, %es
  movw $0x0000, %bx
  #Retrieve the drive number
  movb (drive), %dl
  # Here we go
  # Start from 002
  movw $0x0002, %cx
  movb $0x00, %dh
loop:
  cmpl $0, %edi
  jle done # Check if we're done copying, leave if we are
  movw $0x0201, %ax # Int 13h, AH = 2, read AL (1) sector(s)
  int $0x13
  movb %cl, %al
  andb $0x3f, %al # Get sector number
  # Check if end of sectors per track
  # Either next sector or next head
  cmpb %al, (sectors_per_track)
  je head_next # Jump to next head
  incb %al # Next sector
  jmp loop_next
head_next:
  movb $0x01, %al # Sector 1, (0 unused)
  incb %dh # Head Number ++
  # Check if end of cyl
  cmpb %dh, (heads_per_cyl)
  jne loop_next
cylinder_next:
  movb $0x00, %dh # Head 0
  # ++cyl
  xchgb %ch, %cl #Exchange ch/cl
  shrb $6, %ch #Shift ch right by 6
  incw %cx
  shlb $6, %ch #Shift ch back
  xchgb %ch, %cl #Exchange back
loop_next:
  andb $0xc0, %cl # Clear sec
  orb %al, %cl # Addsec to cx
  # decrease sectors to read
  decl %edi
  movw %es, %ax # Increase dest seg
  addw $(SSIZE >> 4), %ax
  movw %ax, %es
  jmp loop
done:                 # Protected Mode
  lgdt gdt_desc         # Load GDT
  mov %cr0, %eax        # set PE bit of CR0 to 1
  or $1, %eax
  mov %eax, %cr0
  ljmp $0x08, $clear
clear:
  movw $DATA_SELECTOR, %ax           # Fix segments
  # movw $DATA_SELECTOR, %ax # setting up registers and farjumping  
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs
  movw %ax, %gs
  movw $DISPLAY_SEGMENT, %bx
  movw %bx, %es
  movw $STACK_SEGMENT, %ax
  movw %ax, %ss
  movw $STACK_POINTER, %sp
  
  # Clear screen
  movw $0x0a00, %ax     # Fill with black background / green foreground
  movw $2000, %cx       # Number of characters (80x25 screen in text mode = 2000)
  xorw %di, %di         # DI = 0
  rep stosw
  
  movb $0x4b, %es:(0x0) # Write 'K' in the upper left corner of the screen

forever:
  jmp forever # Loop forever



# GDT code mostly yoinked from 
# http://www.osdever.net/tutorials/view/the-world-of-protected-mode
gdt:
gdt_null:
  .quad 0
gdt_code:
  .word 0xFFFF
  .word 0
  .byte 0
  .byte 0b10011010
  .byte 0
gdt_data:
  .word 0xFFFF
  .word 0
  .byte 0
  .byte 0b10010010
  .byte 0b11001111
  .byte 0
gdt_end:
gdt_desc:
  .byte gdt_end - gdt
  .word gdt
# IDENTIFY
# I have been told that this isnt what the assignment wants me to do, so I'm leaving it commented for posterity
# .fill 510 - (. - _start), 0
# .word 0xAA55
