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
#ifndef __PANIC_H__
#define __PANIC_H__

#define PANIC_MOUNT      1,1,-1
#define PANIC_EXCEPTION  1,3,1,-1
#define PANIC_IPCOVF     1,3,3,-1

void panic2(int mode, ...)  __attribute__ ((noreturn));

#endif
