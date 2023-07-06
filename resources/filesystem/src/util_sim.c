/*
 *
 * Util functions to be used when testing the filesystem in Linux/ windows.
 *
 * Various utility functions that can be linked with both the kernel
 * and user code.
 *
 * K&R = "The C Programming Language", Kernighan, Ritchie
 */

#include "common.h"
#include "print.h"
#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> /* atoi */

/*
 * Convert a double to a string with the specified precision
 * (ignores sign; double is always treated as positive)
 */
void dtoa(double dbl, char *s, int precision) {

	int i;
	long unsigned int integer, multiplier;

	if (dbl < 0)
		dbl *= (-1);

	i = 0;
	integer = (long unsigned int)dbl;
	multiplier = 10;

	do {
		s[i++] = integer % 10 + '0';
		integer /= 10;

	} while (integer > 0);

	s[i] = '\0';
	reverse(s);

	if (!precision)
		return;

	s[i++] = ',';
	s = &s[i];

	multiplier = 1;
	for (i = 0; i < precision; i++)
		multiplier *= 10;

	dbl -= (long int)dbl;
	integer = dbl * multiplier;

	multiplier = 10;
	i = 0;

	do {
		s[i++] = integer % multiplier + '0';
		integer /= 10;

	} while (integer > 0);

	s[i] = '\0';
	reverse(s);
}

int scrwrite(void *drop, char c) {
	return printf("%c", c);
}

int scrprintf(int line, int col, char *in, ...) {
	struct output out = {.write = scrwrite, /* write function */
	                     .data = NULL};
	va_list args;

	va_start(args, in);
	return uprintf(&out, in, args);
}

/* Read the character stored at location (x,y) on the screen */
int peek_screen(int x, int y) {
	return -1;
}

void clear_screen(int minx, int miny, int maxx, int maxy) {
}

/* scroll screen */
void scroll(int minx, int miny, int maxx, int maxy) {
}

/* Simple delay loop to slow things down */
void delay(int n) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	int i, j;

	for (i = 0; i < n; i++)
		j = i * 11;
#pragma GCC diagnostic pop
}

/* Read the pentium time stamp counter */
unsigned long long int get_timer(void) {
	return 0;
}

/* Convert an ASCII string (like "234") to an integer */
int atoi(const char *s) {
	int n;
	for (n = 0; *s >= '0' && *s <= '9'; n = n * 10 + *s++ - '0')
		;
	return n;
}

/*
 * Functions from Kerninghan/Ritchie - The C Programming Language
 */

/* Convert an integer to an ASCII string, Page 64 */
void itoa(unsigned int n, char *s) {
	int i;

	i = 0;
	do {
		s[i++] = n % 10 + '0';
	} while ((n /= 10) > 0);
	s[i++] = 0;
	reverse(s);
}

/* Convert an integer to an ASCII string, base 16 */
void itohex(unsigned int n, char *s) {
	int i, d;

	i = 0;
	do {
		d = n % 16;
		if (d < 10)
			s[i++] = d + '0';
		else
			s[i++] = d - 10 + 'a';
	} while ((n /= 16) > 0);
	s[i++] = 0;
	reverse(s);
}

/* Reverse a string, Page 62 */
void reverse(char *s) {
	int c, i, j;

	for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

/* Get the length of a null-terminated string, Page 99 */
int strlen(const char *s) {
	int n;
	for (n = 0; *s != '\0'; s++)
		n++;
	return n;
}

void strcpy(char *dest, char *source) {
	bcopy(source, dest, strlen(source) + 1);
}

/*
 * strncmp:
 * Compares at most n bytes from the strings s and t.
 * Returns >0 if s > t, <0 if s < t and 0 if s == t.
 */
int strncmp(const char *s, const char *t, size_t n) {
	for (; n-- > 0 && *s == *t; ++s, ++t)
		if (*s == '\0')
			return 0;
	return *s - *t;
}

/*
 * strncpy:
 * Coptes at most size characters from src to dest, and '\0'
 * terminates the result iff src is less than len characters long.
 * Returns dest.
 */
char *strncpy(char *dest, const char *src, int len) {
	while (len-- > 0 && (*dest++ = *src++))
		;
	return dest;
}

/*
 * strlcpy:
 * Copies at most size-1 characters from src to dest, and '\0'
 * terminates the result. One must include a byte for the '\0' in
 * dest. Returns the length of src (so that truncation can be
 * detected).
 */
int strlcpy(char *dest, const char *src, int size) {
	int n, m;

	n = m = 0;
	while (--size > 0 && (*dest++ = *src++))
		++n;
	*dest = '\0';
	while (*src != '\0')
		++src, ++m;
	return n + m;
}

/* return TRUE if string s1 and string s2 are equal */
int same_string(char *s1, char *s2) {
	while ((*s1 != 0) && (*s2 != 0)) {
		if (*s1 != *s2)
			return FALSE;
		s1++;
		s2++;
	}
	return (*s1 == *s2);
}

/* Block copy: copy size bytes from source to dest.
 * The arrays can overlap */
void bcopy(const char *source, char *destin, int size) {
	int i;

	if (size == 0)
		return;

	if (source < destin) {
		for (i = size - 1; i >= 0; i--)
			destin[i] = source[i];
	}
	else {
		for (i = 0; i < size; i++)
			destin[i] = source[i];
	}
}

/* Zero out size bytes starting at area */
void bzero(char *area, int size) {
	int i;

	for (i = 0; i < size; i++)
		area[i] = 0;
}

/* Read byte from I/O address space */
uint8_t inb(int port) {
	return 0;
}

/* Write byte to I/O address space */
void outb(int port, uint8_t data) {
}
