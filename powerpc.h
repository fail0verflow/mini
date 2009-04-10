/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	PowerPC support code

Copyright (C) 2008, 2009 	Sven Peter <svenpeter@gmail.com>

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
#ifndef	__POWERPC_H__
#define	__POWERPC_H__	1

#include "ipc.h"

void powerpc_upload_stub(u32 entry);
void powerpc_hang();
void powerpc_reset();
void powerpc_ipc(volatile ipc_request *req);

#endif

