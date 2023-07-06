# NB:  this does not replace bootblock.s, which you have been
# given as part of the pre-code. It is only meant as a hint, and
# to show a few examples of some useful code constructions.
#
# Several of the things demonstrated here might be done differently
# (more compact, more elegant, ++), but this is at least a starting
# point that you may use.
#
# jmb

  
  .equ    BOOT_SEGMENT,      0x07c0
  .equ    DISPLAY_SEGMENT,   0xb800
  
# You still need to decide where to put the stack
# .equ    STACK_SEGMENT,     0xXXXX
# .equ    STACK_POINTER,     0xXXXX

.text
.globl  _start
.code16

_start:
  jmp  over
os_size:      
  .word 0
over:
  # setup stack
  movw  $STACK_SEGMENT, %ax
  movw  %ax, %ss
  movw  $STACK_POINTER, %sp

  # setup data segment
  movw  $BOOT_SEGMENT, %ax
  movw  %ax, %ds
  
  # -------------------
  #   Example of a simple if-construction,
  #   if (a == 2)
  #      a  = 3; 
  #     
  
  movw  $3, %ax          # set test value for a
  cmpw  $2, %ax
  jne   nope
  movw  $3, %ax
nope:  


  # -------------------
  #    Example of 
  #  for (i = 0; i < 5; i++) 
  #      a = i;
  # 
  movw  $0, %cx          # Use CX as variable 'i'
loop1:  
  cmpw  $5, %cx    
  jge   loop1done        # Jump if greater than or equal
  movw  %cx, %ax         # Use AX as variable 'a'
  incw  %cx
  jmp   loop1
loop1done:
  # Notice that cmpw $5, %cx would have been
  # cmp cx, 5 in Intel syntax. jge in this case
  # therefore means "jump to loop1done if %cx is greater
  # than or equal to 5"

  # ------------------
  #  Here, I've added a call to print, such that the code resembles
  #  something like this:   
  #  for (i = 0; i < 5; i++) {
  #      a = i;
  #      print(mystring);   /* Mystring is a char/string pointer*/
  #  }

  movw  $0, %cx          # Use CX as variable 'i'
loop1b:  
  cmpw  $5, %cx
  jge   loop1bdone       # Jump if greater than or equal
  movw  %cx, %ax         # Use AX as variable 'a'
  
  movl  $mystring, %esi  # test string for debug
  call  print            # call print routine
  
  incw  %cx
  jmp   loop1b
loop1bdone:  

  
  # ---------------------
  #  a = 0;
  #  do {
  #     a = a + 1; 
  #  } while (a < 10);

  movw  $0, %ax
loop2:
  incw  %ax
  cmpw  $10, %ax         
  jl    loop2  



  # -------------------
  # Save value before function call

  pushw %ax              # AX contains something we don't want to lose
  movw  $mystring, %si
  call  print
  popw  %ax
  
  
  # say hello to user
  movl  $hellostring,%esi
  call  print
  
forever:
  jmp   forever

# routine to print a zero terminated string pointed to by esi
# Overwrites:   AX, DS, BX
print:
  movw  $BOOT_SEGMENT, %ax
  movw  %ax, %ds
print_loop:
  lodsb
  cmpb  $0,%al
  je    print_done
  movb  $14, %ah
  movl  $0x0002, %ebx
  int   $0x10
  jmp   print_loop
print_done:
  retw

# messages

mystring:  
  .asciz "test.\n\r"
hellostring:  
  .asciz "Hi there.\n\r"

