/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	panic flash codes

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __PANIC_H__
#define __PANIC_H__

#define PANIC_MOUNT      1,1,-1
#define PANIC_EXCEPTION  1,3,1,-1
#define PANIC_IPCOVF     1,3,3,-1
#define PANIC_PATCHFAIL  1,3,3,3,-1

void panic2(int mode, ...)  __attribute__ ((noreturn));

#endif

