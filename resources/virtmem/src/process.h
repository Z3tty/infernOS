
#ifndef __PROCESS_H_
#define __PROCESS_H_

/* In reality this symbol comes from Makefile.
 * This provides pre-compile time reference to the symbol to avoid linter warnings. 
 * In other words, these symbols are discarded and overwritten by Makefile at compile-time.
 */
#ifndef PROCESS_START
#define PROCESS_START 0x1000000 // virtual address of processes
#endif /* PROCESS_START */

#endif /* __PROCESS_H_ */
