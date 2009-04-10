/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	panic flash codes

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#include "types.h"
#include "utils.h"
#include "start.h"
#include "hollywood.h"
#include <stdarg.h>

#define PANIC_ON	200000
#define PANIC_OFF	300000
#define PANIC_INTER	1000000

// figure out a use for mode...

void panic2(int mode, ...)
{
	int arg;
	va_list ap;

	clear32(HW_GPIO1OUT, HW_GPIO1_SLOT);
	clear32(HW_GPIO1DIR, HW_GPIO1_SLOT);
	clear32(HW_GPIO1OWNER, HW_GPIO1_SLOT);

	while(1) {
		va_start(ap, mode);
		
		while(1) {
			arg = va_arg(ap, int);
			if(arg < 0)
				break;
			set32(HW_GPIO1OUT, HW_GPIO1_SLOT);
			udelay(arg * PANIC_ON);
			clear32(HW_GPIO1OUT, HW_GPIO1_SLOT);
			udelay(PANIC_OFF);
		}
		
		va_end(ap);
		
		udelay(PANIC_INTER);
	}
}
