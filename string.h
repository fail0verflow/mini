#ifndef _STRING_H_
#define _STRING_H_

#include "types.h"

char *strcpy(char *, const char *);
char *strncpy(char *, const char *, size_t);
int strcmp(const char *, const char *);
int strncmp(const char *p, const char *q, size_t n);
size_t strlen(const char *);
size_t strnlen(const char *, size_t);
char *strchr(const char *s, int c);
void *memset(void *, int, size_t);
void *memcpy(void *, const void *, size_t);
int memcmp(const void *s1, const void *s2, size_t n);

#endif
