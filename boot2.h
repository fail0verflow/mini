/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	boot2 chainloader

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __BOOT2_H__
#define __BOOT2_H__

#include "ipc.h"

u32 boot2_run(u32 tid_hi, u32 tid_lo);
void boot2_init();
u32 boot2_ipc(volatile ipc_request *req);

#endif

