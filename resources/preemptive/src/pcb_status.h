#ifndef PCB_STATUS_H
#define PCB_STATUS_H
/*
 * This header is used by assembly as well as C code. Therefore, only
 * preprocessor directives should be used in this file.
 */

/* Process/thread status */
#define STATUS_FIRST_TIME 0
#define STATUS_READY 1
#define STATUS_BLOCKED 2
#define STATUS_EXITED 3

#endif /* !PCB_STATUS_H */
