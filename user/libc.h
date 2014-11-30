#ifndef _LIBC_H_
#define _LIBC_H_

#include "sys.h"

extern void puts(char *p);
extern char* gets();
extern void* malloc(long size);
extern void free(void*);
extern void* realloc(void* ptr, long newSize);
extern void putdec(unsigned long v);
extern void puthex(long v);
extern long readFully(long fd, void* buf, long length);
extern long writeFully(long fd, void* buf, long length);

/* string operations */
extern int strcmp(char *s1, char *s2);
extern int strtok(char *cmd, char target);
extern int strlen(char* str);
extern int atoi(char *str);

/* number operations */
extern void dec2hex(unsigned dec, char *buf, int digits);

void memset(void* p, int val, long sz);
void memcpy(void* dest, void* src, long n);

// /* permission */
// extern int checkPermission(long fd, int mask);

#endif
