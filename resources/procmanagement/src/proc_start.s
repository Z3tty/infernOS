.text
.code32
  .align 4


.globl  _start

# void _start(void)
#
# Used as a reliable entry and
# exit point for processes
#
# Jumps to function "main",
# and if that returns, it
# will do an exit to properly
# end the process.
_start:
  call  main
  call  exit
