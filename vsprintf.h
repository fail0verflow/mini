#ifndef __VSPRINTF_H__
#define __VSPRINTF_H__

#include <stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list args);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

#endif

