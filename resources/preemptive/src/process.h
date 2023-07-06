#ifndef __PROCESS_H_
#define __PROCESS_H_

/* In reality this symbol comes from Makefile.
 * This provides pre-compile time reference to the symbol to avois linter warnings. 
 * In other words, these symbols are discarded and overwritten by Makefile at compile-time.
 */
#ifndef PROC1_ADDR
#define PROC1_ADDR 0x5000
#endif /* PROC1_ADDR */

/* In reality this symbol comes from Makefile.
 * This provides pre-compile time reference to the symbol to avois linter warnings. 
 * In other words, these symbols are discarded and overwritten by Makefile at compile-time.
 */
#ifndef PROC2_ADDR
#define PROC2_ADDR 0x7000
#endif /* PROC2_ADDR */

#endif /* __PROCESS_H_ */
