/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	PowerPC support code

Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>
Copyright (C) 2009			Andre Heider "dhewg" <dhewg@wiibrew.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __POWERPC_H__
#define __POWERPC_H__

#include "ipc.h"

void powerpc_upload_stub(u32 entry);
void powerpc_hang(void);
void powerpc_reset(void);
void powerpc_ipc(volatile ipc_request *req);

#endif

