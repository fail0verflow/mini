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
#include "types.h"
#include "powerpc.h"
#include "hollywood.h"
#include "utils.h"
#include "start.h"
#include "gecko.h"

void powerpc_upload_stub(u32 entry)
{
	u32 i;

	set32(HW_EXICTRL, EXICTRL_ENABLE_EXI);

	// lis r3, entry@h
	write32(EXI_BOOT_BASE + 4 * 0, 0x3c600000 | entry >> 16);
	// ori r3, r3, entry@l
	write32(EXI_BOOT_BASE + 4 * 1, 0x60630000 | (entry & 0xffff));
	// mtsrr0 r3
	write32(EXI_BOOT_BASE + 4 * 2, 0x7c7a03a6);
	// li r3, 0
	write32(EXI_BOOT_BASE + 4 * 3, 0x38600000);
	// mtsrr1 r3
	write32(EXI_BOOT_BASE + 4 * 4, 0x7c7b03a6);
	// rfi
	write32(EXI_BOOT_BASE + 4 * 5, 0x4c000064);

	for (i = 6; i < 0x10; ++i)
		write32(EXI_BOOT_BASE + 4 * i, 0);

	set32(HW_DIFLAGS, DIFLAGS_BOOT_CODE);

	gecko_printf("disabling EXI now...\n");
	clear32(HW_EXICTRL, EXICTRL_ENABLE_EXI);
}

void powerpc_hang()
{
	clear32(HW_RESETS, 0x30);
	udelay(100);
	set32(HW_RESETS, 0x20);
	udelay(100);
}

void powerpc_reset()
{
	write32(0xD800034, 0x40000000);
	clear32(HW_RESETS, 0x30);
	udelay(100);
	set32(HW_RESETS, 0x20);
	udelay(100);
	set32(HW_RESETS, 0x10);
	udelay(100000);
	set32(HW_EXICTRL, EXICTRL_ENABLE_EXI);
}
