/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	random utilities

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "types.h"
#include "utils.h"
#include "gecko.h"
#include "vsprintf.h"
#include "start.h"
#include "gpio.h"
#include "hollywood.h"

#include <stdarg.h>

#if defined(CAN_HAZ_USBGECKO) && !defined(LOADER) && !defined(NDEBUG)
static char ascii(char s) {
  if(s < 0x20) return '.';
  if(s > 0x7E) return '.';
  return s;
}

void hexdump(const void *d, int len) {
  u8 *data;
  int i, off;
  data = (u8*)d;
  for (off=0; off<len; off += 16) {
    gecko_printf("%08x  ",off);
    for(i=0; i<16; i++)
      if((i+off)>=len) gecko_printf("   ");
      else gecko_printf("%02x ",data[off+i]);

    gecko_printf(" ");
    for(i=0; i<16; i++)
      if((i+off)>=len) gecko_printf(" ");
      else gecko_printf("%c",ascii(data[off+i]));
    gecko_printf("\n");
  }
}
#endif

int sprintf(char *buffer, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsprintf(buffer, fmt, args);
	va_end(args);
	return i;
}

void udelay(u32 d)
{
	// should be good to max .2% error
	u32 ticks = d * 19 / 10;

	if(ticks < 2)
		ticks = 2;

	u32 now = read32(HW_TIMER);

	u32 then = now  + ticks;

	if(then < now) {
		while(read32(HW_TIMER) >= now);
		now = read32(HW_TIMER);
	}

	while(now < then) {
		now = read32(HW_TIMER);
	}
}

void panic(u8 v)
{
	while(1) {
		debug_output(v);
		set32(HW_GPIO1BOUT, GP_SLOTLED);
		udelay(500000);
		debug_output(0);
		clear32(HW_GPIO1BOUT, GP_SLOTLED);
		udelay(500000);
	}
}

