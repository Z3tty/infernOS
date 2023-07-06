#ifndef TH_H
#define TH_H

void clock_thread(void);

/* Threads to test the condition variables and locks */
void thread2(void);
void thread3(void);
/* Threads to test semaphores */
void num(void);
void scroll_th(void);
void caps(void);

/* Threads to test barriers */
void barrier1(void);
void barrier2(void);
void barrier3(void);

/* Threads to test floating point context switches and locks */
void mcpi_thread0(void);
void mcpi_thread1(void);
void mcpi_thread2(void);
void mcpi_thread3(void);

#endif /* !TH_H */
