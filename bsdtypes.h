/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	BSD types compatibility layer for the SD host controller driver.

Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>

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
#ifndef __BSDTYPES_H__
#define __BSDTYPES_H__

#include "types.h"
#include "errno.h"

typedef u32 u_int;
typedef u32 u_int32_t;
typedef u16 u_int16_t;
typedef u8 u_int8_t;
typedef u8 u_char;

typedef u32 bus_space_tag_t;
typedef u32 bus_space_handle_t;

struct device {
	char dv_xname[255];
	void *dummy;
};

#define MIN(a, b) (((a)>(b))?(b):(a))

#define wakeup(...)

#define bzero(mem, size) memset(mem, 0, size)

#define ISSET(var, mask) (((var) & (mask)) ? 1 : 0)
#define SET(var, mask) ((var) |= (mask))

#endif

