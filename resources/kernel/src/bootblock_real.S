# bootblock.s

# .equ symbol, expression
# These directive set the value of the symbol to the expression
  .equ    BOOT_SEGMENT,      0x07c0
  .equ    DISPLAY_SEGMENT,   0xb800
  .equ    KERNEL_SEGMENT,    0x0000
  .equ    KERNEL_OFFSET,     0x1000
  # --
  .equ    DATA_SELECTOR,     0x10
  .equ    CODE_SELECTOR,     8
  .equ    SECTORS,           18 # Lord forgive me
# You need to decide where to put the stack
  .equ    STACK_SEGMENT,     0x080000 # Allocate FFFFh for the stack
  .equ    STACK_POINTER,     0x09f000 # Place the stack high in memory so that it can grow down without too much issue, without overlapping VRAM/VROM

.text                   # Code segment
.globl    _start        # The entry point must be global
.code16                 # Real mode
.org 0x0
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
#
# The first instruction to execute in a program is called the entry
# point. The linker expects to find the entry point in the "symbol" _start
# (with underscore).
#

_start:
  cli                   # Disable interrupts during setup
  # Load GDT
  lgdt (gdt_desc)
  # Set up segment registers to the boot segment
  movw $BOOT_SEGMENT, %ax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs
  movw %ax, %gs
  movw %ax, %ss
  # movw $STACK_SEGMENT, %ss
  ljmp $BOOT_SEGMENT, $after_setup
after_setup:
  # Read the kernel
  jmp read_kernel
.code32                 # Protected Mode
  mov %cr0, %eax        # set PE bit of CR0 to 1
  or $1, %eax
  mov %eax, %cr0
  ljmp $0x08, $clear
clear:
  movw $KERNEL_SEGMENT, %ax           # Fix segments
  # movw $DATA_SELECTOR, %ax # setting up registers and farjumping  
  # movw $BOOT_SEGMENT, %ax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs
  movw %ax, %gs
  movw %ax, %ss
  # ljmp $KERNEL_SEGMENT, $KERNEL_OFFSET
  jmp beyondReservedSpace

kernelSize:
  .word 0               # bootimage will write size of kernel, in sectors

read_kernel:
  # Start reading kernel, https://en.wikipedia.org/wiki/INT_13H#INT_13h_AH=02h:_Read_Sectors_From_Drive
  # This is a BAD way of doing it. It should only work for the sole reason that the kernel is teeny tiny
  # It only reads 16 sectors and the kernel HAS to be there
  # Reworking this isnt "hard", per-se. You could simply keep track of which sector to read by using a register and incrementing it
  # until the entire kernelSize is read.
  # Time is a factor, though.
  sti                    # Re-enable interrupts
  pushw %ax
  pushw %dx
  xor %ax, %ax
  movw %ax, %ds           # set ds to 0
  mov $0x2, %ah          # Int 13h, ah=2h is read sector
  mov $SECTORS, %al      # Int 13h, al=n sectors to read
  mov $0x0, %ch          # Int 13h, ch=0, read from cylinder 0
  mov $2, %cl            # Int 13h, cl=2, second sector, starts at 1
  mov $0x0, %dh          # Int 13h, dh=0, head #0
  xor %bx, %bx           
  movw %bx, %es           # set es to 0
  # movw $BOOT_SEGMENT, %es  # Perchance?
  movw $KERNEL_OFFSET, %bx # I think? Maybe? Its the offset to load sooo
  int $0x13
  # rep movsd, "0xFFFFFFFFFFFFFFFF1000" # over 1mb
  # Disable interrupts and return
  popw %dx
  popw %ax
  cli
  ret              

beyondReservedSpace:
  movw $DISPLAY_SEGMENT, %bx
  movw %bx, %es
  
  # Clear screen
  movw $0x0a00, %ax     # Fill with black background / green foreground
  movw $2000, %cx       # Number of characters (80x25 screen in text mode = 2000)
  xorw %di, %di         # DI = 0
  rep stosw
  
  movb $0x4b, %es:(0x0) # Write 'K' in the upper left corner of the screen

forever:
  jmp forever # Loop forever

# IDENTIFY
# I have been told that this isnt what the assignment wants me to do, so I'm leaving it commented for posterity
# .fill 510 - (. - _start), 0
# .word 0xAA55
