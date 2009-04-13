/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	boot2 chainloader

Copyright (c) 2008			Nuke - <wiinuke@gmail.com>
Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>
Copyright (C) 2009			Andre Heider "dhewg" <dhewg@wiibrew.org>

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
#ifndef __GECKO_H__
#define __GECKO_H__

#include "types.h"

void gecko_init(void);
u8 gecko_enable_console(const u8 enable);
int gecko_printf(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
void gecko_timer_initialize(void);
void gecko_timer(void);

#endif

