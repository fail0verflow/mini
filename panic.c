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
