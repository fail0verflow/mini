/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	boot2 chainloader

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
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
#ifndef __BOOT2_H__
#define __BOOT2_H__

#include "ipc.h"

u32 boot2_run(u32 tid_hi, u32 tid_lo);
void boot2_init();
u32 boot2_ipc(volatile ipc_request *req);

#endif

