#ifndef __START_H__
#define __START_H__

#include "types.h"

void delay(u32 delay);

#define udelay(d) delay(247*(d)/10)

void debug_output(u8 byte);
void panic(u8 code);

#endif
