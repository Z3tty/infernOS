#ifndef COMMON_H
#define COMMON_H

#ifndef NULL
#define NULL ((void*) 0)
#endif

/* Size of a sector on the USB stick */
#define SECTOR_SIZE 512

/* Physical address of the text mode display  */
#define SCREEN_ADDR (short *) 0xb8000

/* Unique integers for each system call  */
enum {
  SYSCALL_YIELD,    /* 0 */
  SYSCALL_EXIT,
  SYSCALL_GETPID,
  SYSCALL_GETPRIORITY,
  SYSCALL_SETPRIORITY,
  SYSCALL_CPUSPEED,  /* 5 */
  SYSCALL_MBOX_OPEN,
  SYSCALL_MBOX_CLOSE,
  SYSCALL_MBOX_STAT,
  SYSCALL_MBOX_RECV,
  SYSCALL_MBOX_SEND,  /* 10 */
  SYSCALL_GETCHAR,
  SYSCALL_READDIR,
  SYSCALL_LOADPROC,
        SYSCALL_FS_FSCK,
        SYSCALL_FS_MKFS,        /* 15 */
        SYSCALL_FS_OPEN,
        SYSCALL_FS_CLOSE,
        SYSCALL_FS_READ,
        SYSCALL_FS_WRITE,
        SYSCALL_FS_LSEEK,       /* 20 */
        SYSCALL_FS_LINK,
        SYSCALL_FS_UNLINK,
        SYSCALL_FS_STAT,
        SYSCALL_FS_MKDIR,
        SYSCALL_FS_CHDIR,       /* 25 */
        SYSCALL_FS_RMDIR,
   SYSCALL_COUNT
};


/*
 * If the expression p fails, print the source file and line number
 * along with the text s. Then hang the os. Processes should not use
 * this macro.
 */
#define ASSERT2(p, s)      		\
do {      				\
  if (!(p)) {    			\
    scrprintf(0, 0, "Assertion failure: %s", s);  \
    scrprintf(1, 0, "file: %s", __FILE__);  	\
    scrprintf(2, 0, "line: %d", __LINE__);  	\
    asm volatile ("cli");  		\
    while (1);  			\
  }    				\
} while(0)

/*
 * The #p tells the compiler to copy the expression p 
 * into a text string and use this as an argument. 
 * => If the expression fails: print the expression as a message. 
 */

#ifndef LINUX_SIM
#define ASSERT(p) ASSERT2(p, #p)
#else
#define ASSERT(p) assert(p)
#endif 

/* Gives us the source file and line number where we decide to hang the os. */
#define HALT(s) ASSERT2(FALSE, s)

/* Typedefs of commonly used types */
typedef enum {
  FALSE, TRUE
} bool_t;

/*
 * stdint.h has typedefs for int8_t, uint8_t etc...
 */
#include <stdint.h>



/*
 * Note that this struct only allocates space for the size element.
 *
 * To use a message with a body of 50 bytes we must first allocate space for 
 * the message:
 *  char space[sizeof(int) + 50];
 * Then we declares a msg_t * variable that points to the allocated space:
 *  msg_t *m = (msg_t *) space;
 * We can now access the size variable:
 *   m->size (at memory location &(space[0]) )
 * And the 15th character in the message:
 *   m->body[14] (at memory location &(space[0]) + sizeof(int) + 14 *
 *   sizeof(char))
 */
struct msg {
  int size;    /* Size of message contents in bytes */
  char body[0];    /* Pointer to start of message contents */
} __attribute__ ((packed));

typedef struct msg msg_t;

/* Return size of header */
#define MSG_T_HEADER_SIZE (sizeof(int))
/* Return size of message including header */
#define MSG_SIZE(m) (MSG_T_HEADER_SIZE + m->size)

/*
 * Structure used for interpreting the process directory in the
 * "filesystem" on the USB stick .
 */
struct directory_t {
  int location;    /* Sector number */
  int size;    /* Size in number of sectors */
};

extern int os_size; /* size of os in disk blocks */

#endif /* !COMMON_H */
