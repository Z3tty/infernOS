# bootblock.s: toy boot loader for COS 318
# Author: David Eisenstat <deisenst@CS.Princeton.EDU>
#
# Note: on the current fishbowl machines (Dell Dimension 9200s),
# INT 0x13 _crashes_ unless the stack is set up the same way
# as it was when we booted.
#
# Revision history:
# 2007-09-15    Support for protected mode and memory >= 1MB
# 2007-09-12    Initial version
##

# The BIOS puts the boot loader here
  .equ  BOOTBLOCK_SEGMENT,  0x07c0
# The boot loader copies itself here to make room for the OS
  .equ  NEW_BOOT_SEGMENT,   0x00e0
# Number of bytes in a sector
  .equ  SECTOR_SIZE,        0x0200
# The boot loader puts the OS here
  .equ  OS_SEGMENT,         0x0100

# GDT descriptors for CS and DS segments
  .equ  KERNEL_CS_DESC,     0x0008
  .equ  KERNEL_DS_DESC,     0x0010

# Setup stack and stack segment (stack grows from 0x9fbfe)
  .equ  STACK_SEGMENT,      0x8fc0
  .equ  STACK_BASEPOINTER,  0xfffe

.text                                   # Code segment
.globl _start                           # The entry point must be global
.code16                                 # Real mode

# Entry point
_start:
  # Jump over the memory that holds global variables
  jmp    beyondReservedSpace


# Reserve space for createimage to write the size of the OS, in sectors
os_size:
  .long  0

# Reserved space for active drive number
activeDrive:
  .byte  0

# Struct used by setDiskInfo to store information about active drive
# Reserved place to store Sectors Per Track
SPT:
  .word  0
# Reserved place to store Heads Per Cylinder
HPC:
  .byte  0


beyondReservedSpace:
  # setup stack
  movw  $STACK_SEGMENT, %ax
  movw  %ax, %ss
  movw  $STACK_BASEPOINTER, %sp
  
  # Set data segment
  movw  $BOOTBLOCK_SEGMENT, %ax
  movw  %ax, %ds
  
  # Move the boot loader to the new location
  # The source is %ds:%si
  movw  $BOOTBLOCK_SEGMENT, %ax
  movw  %ax, %ds
  movw  $0x0000, %si
  # The destination is %es:%di
  movw  $NEW_BOOT_SEGMENT, %ax
  movw  %ax, %es
  movw  $0x0000, %di
  # The number of bytes to be copied is %cx
  # The boot loader occupies one sector, or 512 bytes
  movw  $SECTOR_SIZE, %cx
  # Clear the direction bit so that movsb _increments_ %si and %di
  cld
  # Do it
  rep   movsb
  
  # Jump to the next instruction at the new location
  ljmp  $NEW_BOOT_SEGMENT, $after_move

after_move:
  # Update data segment to new bootblock location
  movw  $NEW_BOOT_SEGMENT, %ax
  movw  %ax, %ds
  
  # Store active drive number, which BIOS has put in DL
  movb  %dl, activeDrive
  
  # Get CHS information about active drive from BIOS
  movw  $0x0800, %ax                    # Read disk parameters
  # Set %es:di to 0
  xorw  %bx, %bx
  movw  %bx, %es
  xorw  %di, %di
  # Invoke BIOS service INT 13h/AH=8h to read disk parameters
  int   $0x13                           # Info about sectors and cylinders is placed in CX, and DH = (num heads - 1)
  
  # Extract sectors
  movw  %cx, %ax
  andw  $0x3f, %ax                      # bitwise-AND with bitmask to extract number of sectors (lower 6 bits of CX)
  movb  %al, SPT                        # Store sector number in SPT

  # Extact heads
  movb  %dh, %al                        # AL = DH, which contains index of last head
  incb  %al                             # Add 1 to get number of heads
  movb  %al, HPC                        # Store heads number in HPC
  
copy_kernel:
  # EDI = remaining number of sectors to copy
  movl  (os_size), %edi
  
  # First sector is copied to $OS_SEGMENT:0x0000
  movw  $OS_SEGMENT, %ax
  movw  %ax, %es
  movw  $0x0000, %bx
  
  # DL = drive to copy sectors from
  movb  (activeDrive), %dl
  
  # Start copying from CHS (0, 0, 2)
  movw  $0x0002, %cx
  movb  $0x00, %dh
  
copy_loop:
  # Check if we have more sectors left to copy
  cmpl  $0, %edi
  jle   after_copy
  
  # Invoke BIOS service INT 13h/AH=2h to read one sector
  movw  $0x0201, %ax      # AH = 0x02 (read sectors), AL = 0x01 (one sector)
  int   $0x13
  
  # If something went wrong, go to copy_error
  jc    copy_error
  cmpb  $0, %ah
  jne   copy_error
  
  # AL = sector number
  movb  %cl, %al
  andb  $0x3f, %al        # Extract sector number from CL
  
  # Check if sector number is equal to SPT
  cmpb  %al, (SPT)
  je    .next_head        # If sector number == SPT, jump to next head
  incb  %al               # Else, we simply increase the sector number
  jmp   .copy_loop_next
  
.next_head:
  movb  $0x01, %al        # sector number = 1
  incb  %dh               # Increase head number first (since head number counts from 0)
  
  # Check if head number is equal to HPC
  cmpb  %dh, (HPC)
  jne   .copy_loop_next   # If head number == HPC, jump to next cylinder
  
.next_cylinder:
  movb  $0x00, %dh
  
  # Increase cylinder number in CX
  xchgb %ch, %cl
  shrb  $6, %ch
  incw  %cx
  shlb  $6, %ch
  xchgb %ch, %cl
    
.copy_loop_next:
  andb  $0xc0, %cl         # Clear sector bits
  orb   %al, %cl           # Add sector number to CX
  
  # Increase destination segment by size of one sector
  movw  %es, %ax
  addw  $(SECTOR_SIZE >> 4), %ax                      
  movw  %ax, %es
  

  # Decrease number of sectors left to copy
  decl  %edi
  
  # Loop
  jmp   copy_loop

copy_error:
  jmp   copy_error

after_copy:
  # Clear screen
  movw  $0x0003, %ax
  int   $0x10
  
  # Hide cursor
  movb  $0x02, %ah                      # Move cursor
  xorb  %bh, %bh                        # page number
  movb  $25, %dh                        # row on screen to move cursor to (y value)
  movb  $80, %dl                        # column on screen to move cursor to    (x value)
  int   $0x10                           # Interrupt
  
  # Turn on address line 20
  # See http://www.win.tue.nl/~aeb/linux/kbd/A20.html
  movb  $0x02, %al
  outb  %al, $0x92
  ## Prepare for protected mode
  # Disable interrupts
  cli
  # Load the global descriptor table
  lgdt  gdt
  
  # Turn on Protection Enable bit (PE)
  movl  %cr0, %eax
  orl   $0x00000001, %eax
  movl  %eax, %cr0
  
  # A long jump is needed for entering protected mode properly!
  ljmp  $KERNEL_CS_DESC, $after_protect + (NEW_BOOT_SEGMENT<<4)


# In protected mode now
.code32
after_protect:
  # Set up the data segments for the OS
  movw   $KERNEL_DS_DESC, %ax
  movw   %ax, %ds
  movw   %ax, %es
  movw   %ax, %fs
  movw   %ax, %gs
  
  # Set up the stack
  movw   %ax, %ss
  movl   $((STACK_SEGMENT << 4) + STACK_BASEPOINTER), %esp
  
  # Transfer control to the OS
  ljmp   $KERNEL_CS_DESC, $(OS_SEGMENT << 4)


# Global Descriptor Table
# See http://www.osdever.net/tutorials/view/protected-mode for more information about the global descriptor table
gdt:
    # The index of the last byte in the gdt
    .word 0x0017
    # The pointer to the start of the gdt
    .long gdt_contents + (NEW_BOOT_SEGMENT << 4)


# The GDT must be aligned
.balign 16
gdt_contents:
    # The first entry is zero
    .long 0x00000000
    .long 0x00000000
    
  ## OS code segment descriptor ##
    # Base  = 0x00000000
    # Limit = 0xfffff
    # Present = 1
    # Privilege = 0
    # Code or data = 1
    # Executable = 1
    # Conforming = 0
    # Readable = 1
    # Accessed = 0
    # Granularity = 1 (4K byte pages)
    # Default Size = 1 (USE32)
    
    # limit bits 7:0, 15:8
    .byte 0xff
    .byte 0xff
    
    # base bits 7:0, 15:8, 23:16
    .byte 0x00
    .byte 0x00
    .byte 0x00
    
    # present (0x80)
    # privilege level 0 (0x00/0x60)
    # code or data (0x10)
    # code (0x08)
    # non-conforming (0x00/0x04)
    # readable (0x02)
    .byte 0x9a
    
    # granularity = 4 KiB pages (0x80)
    # default size = 32 bit (0x40)
    # limit bits 19:16
    .byte 0xcf
    
    # base bits 31:24
    .byte 0x00
  
  ## OS data segment descriptor ##
    # Base  = 0x00000000
    # Limit = 0xfffff
    # Present = 1
    # Privilege = 0
    # Code or data = 1
    # Executable = 0
    # Expansion direction = 0 (segment grows upwards)
    # Writable = 1
    # Accessed = 0
    # Granularity = 1 (4K byte pages)
    # Default Size = 1 (USE32)
    
    # limit bits 7:0, 15:8
    .byte 0xff
    .byte 0xff
    
    # base bits 7:0, 15:8, 23:16
    .byte 0x00
    .byte 0x00
    .byte 0x00
    
    # present (0x80)
    # privilege level 0 (0x00/0x60)
    # code or data (0x10)
    # data (0x00/0x08)
    # non-conforming (0x00/0x04)
    # writable (0x02)
    .byte 0x92
    
    # granularity = 4 KiB pages (0x80)
    # default size = 32 bit (0x40)
    # limit bits 19:16
    .byte 0xcf
    
    # base bits 31:24
    .byte 0x00
