#ifndef UTIL_H
#define UTIL_H

#include "common.h"

#include <stddef.h> /* for size_t */

void clear_screen(int minx, int miny, int maxx, int maxy);
void scroll(int minx, int miny, int maxx, int maxy);
int peek_screen(int x, int y);

void ms_delay(uint32_t msecs);
unsigned long long int get_timer(void);

int atoi(const char *s);
void dtoa(double dbl, char *s, int s_len);
void itoa(uint32_t n, char *s);
void itohex(uint32_t n, char *s);

int scrprintf(int line, int col, char *in, ...);
int rsprintf(char *in, ...);
void invert_color(int line, int col);

void reverse(char *s);
int strlen(const char *s);

void strcpy(char *dest, char *source);
int strncmp(const char *s, const char *t, size_t n);
char *strncpy(char *dest, const char *src, int len);
int strlcpy(char *dest, const char *src, int size);

int same_string(char *s1, char *s2);

void bcopy(const char *source, char *destin, int size);
void bzero(char *a, int size);

uint8_t inb(int port);
uint16_t inw(int port);
uint32_t inl(int port);
void outb(int port, uint8_t data);
void outw(int port, uint16_t data);
void outl(int port, uint32_t data);

uint32_t ntohl(uint32_t data);
uint32_t htonl(uint32_t data);
uint16_t ntohs(uint16_t data);
uint16_t htons(uint16_t data);

void srand(uint32_t seed);
int rand(void);

#endif /* !UTIL_H */
