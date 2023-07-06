.equ BOOT_SEGMENT,  0x07c0  # BIOS puts the loader here
.equ NEW_BOOT_SEG,  0x00e0  # copy here to make room for OS
.equ SECTOR_SIZE,   0x0200  # Size of a sector in bytes
.equ OS_SEGMENT,    0x0100  # Location of the OS for the loader
.equ KERNEL_CS_DC,  0x0008  # GDT Descriptor for code segment
.equ KERNEL_DS_DC,  0x0010  # GDT Descriptor for data segment
.equ STACK_SEGMENT, 0x8fc0  # Grows from 0x9fbfe
.equ STACK_BASEPTR, 0xfffe  # Stack base pointer

.text                       # Code segment
.globl _start               # Global entrypoint
.code16                     # Real mode

_start:                     # ENTRY
    jmp beyondReservedSpace # jump over the mem that holds globals

os_size:
    .long 0                 # Holder for the imager to write to
active_drive:
    .byte 0                 # Current active drive
SPT:
    .word 0                 # Sectors per Track (setDiskInfo)
HPC:
    .byte 0                 # Heads per Cylinder

beyondReservedSpace:
                            # Set up stack
    movw $STACK_SEGMENT, %ax
    movw %ax, %ss
    movw $STACK_BASEPTR, %sp
                            # Set up data segment
    movw $BOOT_SEGMENT, %ax
    movw %ax, %ds
                            # Move bootloader to new loc (source %ds:%si)
    movw $BOOT_SEGMENT, %ax
    movw %ax, %ds
    movw $0x0000, %si
                            # Destination %es:%di
    movw $NEW_BOOT_SEG, %ax
    movw %ax, %es
    movw $0x0000, %di
                            # Sectors to be moved is stored in %cx
    movw $SECTOR_SIZE, %cx
                            # Clear direction bit for movsb to increment
    cld
    rep movsb
                            # Jump to next instr
    ljmp $NEW_BOOT_SEG, $after_move

after_move:
                            # Update DS
    movw $NEW_BOOT_SEG, %ax
    movw %ax, %ds
                            # Store active drive number from BIOS
    movb %dl, active_drive
                            # Get CHS info
    movw $0x0800, %ax       # 0x0800 is query info
    xorw %bx, %bx           # Set %es:%di to 0
    movw %bx, %es
    xorw %di, %di
    int $0x13               # HW INT 13/ AH 0x08 Read Disk
                            # Extract sectors
    movw %cx, %ax
    andw $0x3f, %ax         # Bitmask out the lower 6 of %cx
    movb %al, SPT
                            # Extract heads
    movb %dh, %al           # AH = DL which is last head idx
    incb %al                # increment for num of heads
    movb %al, HPC

copy_kernel:
    movl (os_size), %edi    # EDI = remaining sectors
    movw $OS_SEGMENT, %ax   # First sector is moved to OS:0x0000
    movw %ax, %es
    movw $0x0000, %bx
    movb active_drive, %dl  # DL = Drive to copy from
    movw $0x0002, %cx       # Start from CHS (0, 0, 2)
    movb $0x00, %dh

copy_loop:
    cmpl $0, %edi           # Check if done
    jle after_copy
    movw $0x0201, %ax       # HW INT 13 Read sector
    int $0x13
    jc copy_error           # Jump out on error
    cmpb $0, %ah
    jne copy_error
    movb %cl, %al           # AL = Sector number
    andb $0x3f, %al
    cmpb %al, (SPT)         # check if done with head
    je .next_head
    incb %al
    jmp .copy_loop_next

.next_head:
    movb $0x01, %al         # Sector back to 1
    incb %dh                # increase head index
    cmpb %dh, (HPC)         # Check if done with cylinder
    jne .copy_loop_next

.next_cylinder:
    movb $0x00, %dh         # Heads count from 0
    xchgb %ch, %cl          # increase cyl num in %cx
    shrb $6, %ch
    incw %cx
    shlb $6, %ch
    xchgb %ch, %cl

.copy_loop_next:
    andb $0xc0, %cl         # Clear sector bits
    orb %al, %cl            # Add sec num to %cx
    movw %es, %ax           # Increase dest seg by sec size
    addw $(SECTOR_SIZE >> 4), %ax
    movw %ax, %es
    decl %edi               # Decrease sectors to copy by 1
    jmp copy_loop           # Repeat until done

copy_error:
    jmp copy_error          # Hang on error

after_copy:
    movw $0x0003, %ax
    int $0x10               # HW INT 10; AX 0003; Clear screen
    movb $0x02, %ah         # Move cursor
    xorb %bh, %bh           # Page number
    movb $25, %dh           # Y of cursor
    movb $80, %dl           # X of cursor
    int $0x10               # HW INT 10; AH 02; Move cursor (DX)

    movb $0x02, %al         # A20 (This accursed relic gotta die man)
    outb %al, $0x92

    cli                     # Disable interrupts, moving to protected mode
    lgdt gdt                # Load the global descriptor table
    movl %cr0, %eax         # Turn on the PE bit
    orl $0x00000001, %eax
    movl %eax, %cr0
                            # LJUMP to properly enter protected mode
    ljmp $KERNEL_CS_DC, $after_protect + (NEW_BOOT_SEG << 4)

.code32 # Protected mode
after_protect:
    movw $KERNEL_DS_DC, %ax # Setup OS data segments
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    movw %ax, %ss           # Setup stack
    movl $((STACK_SEGMENT << 4) + STACK_BASEPTR), %esp
                            # Transfer control to OS
    ljmp $KERNEL_CS_DC, $(OS_SEGMENT << 4)

gdt:                        # Global Descriptor Table
    .word 0x0017            # Index of last GDT byte
                            # ptr to start of GDT
    .long gdt_content + (NEW_BOOT_SEG << 4)
.balign                     # GDT MUST be aligned
gdt_content:
    .long 0x00000000        # First entry is 0
    .long 0x00000000
                            #!! OS code segment descriptor !!
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
    .byte 0xff              # limit bits 7:0 15:8
    .byte 0xff
    .byte 0x00              # base bits 7:0 15:8 23:16
    .byte 0x00
    .byte 0x00
    .byte 0x9a              # present x80 priv x00 code/data x10 code x08 nconf x00/04 read x02
    .byte 0xcf              # granularity 4KiB x80 default size 32bit x40 limit bits 19:16
    .byte 0x00              # base bits 31:24
                            #!! OS data segment descriptor !!
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
    .byte 0xff              # limit hits 7:0 15:8
    .byte 0xff
    .byte 0x00              # base bits 7:0 15:8 23:16
    .byte 0x00
    .byte 0x00
    .byte 0x92              # present x80 priv x00 code/data 0x10 data x00/08 nconf x00/04 write x02
    .byte 0xcf              # granularity 4KiB x80 default size 32bit x40 limit bits 19:16
    .byte 0x00              # base bits 31:24
