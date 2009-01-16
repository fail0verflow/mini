#ifndef __GECKO_H__
#define __GECKO_H__

#include "types.h"

void gecko_flush(void);
int gecko_isalive(void);
int gecko_recvbuffer(void *buffer, u32 size);
int gecko_sendbuffer(const void *buffer, u32 size);
int gecko_recvbuffer_safe(void *buffer, u32 size);
int gecko_sendbuffer_safe(const void *buffer, u32 size);
int gecko_putchar(int c);
int gecko_getchar(void);
int gecko_puts(const char *s);
int gecko_printf( const char *fmt, ...) __attribute__((format (printf, 1, 2)));
void gecko_init(void);

#endif
