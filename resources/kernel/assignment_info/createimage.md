
## createimage

## NAME
createimage - create operating system image suitable for placement on a boot disk.

## SYNOPSIS
createimage \[--extended\] \[--vm\] \<bootblock\> \<file\> ...

## DESCRIPTION
This manual page documents the createimage program used to produce an image suitable for placement on a boot disk. When run, the result is placed in a file called image residing in the directory the command was invoked from. The image can be placed on a boot disk (e.g. /dev/fd0) by issuing 

    cat image > /dev/fd0 

on the shell command-line, assuming that you have supplied the bootblock code as one of the executable components.

createimage parses each of the given executable files according to the ELF specification. Thus, the executable files must be compliant to the ELF standard of position independent linking.

The format of the image is fairly simple. The first 512 bytes of the file contains the code for the bootblock. The memory image of the entire OS follows the bootblock. The size of the OS (in sectors) is stored as a "short int" (2 bytes) at location 2 (counting from 0) in the image.

## OPTIONS
--extended
Print extended information useful for debugging the operating system image, process placement in memory and so on. Provides detailed information on the size and position of each executable component in the image file.

--vm 
This option tells createimage to produce an image that can be easily tailored to a virtual memory operating system. A structure, called directory , is placed at the end of the image file, describing where in physical  memory the processes should be placed.

## SEE ALSO
objdump(1), gcc(1), ld(1), gas(1)

Check the ELF Documentation available at the course homepage for details on parsing of the executable components.

## BUGS
The --extended switch must be placed before the --vm switch 
