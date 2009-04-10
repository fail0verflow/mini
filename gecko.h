#ifndef __GECKO_H__
#define __GECKO_H__

#include "types.h"

void gecko_init(void);
u8 gecko_enable_console(const u8 enable);
int gecko_printf(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
void gecko_timer_initialize(void);
void gecko_timer(void);

#endif
