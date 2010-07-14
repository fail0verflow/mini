/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	PowerPC support code

Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>
Copyright (C) 2009			Andre Heider "dhewg" <dhewg@wiibrew.org>
Copyright (C) 2010			Alex Marshall <trap15@raidenii.net>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "types.h"
#include "memory.h"
#include "powerpc.h"
#include "powerpc_elf.h"
#include "hollywood.h"
#include "utils.h"
#include "string.h"
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
	set32(HW_AHBPROT, 0xFFFFFFFF);

	gecko_printf("disabling EXI now...\n");
	clear32(HW_EXICTRL, EXICTRL_ENABLE_EXI);
}

void powerpc_hang(void)
{
	clear32(HW_RESETS, 0x30);
	udelay(100);
	set32(HW_RESETS, 0x20);
	udelay(100);
}

void powerpc_reset(void)
{
	// enable the broadway IPC interrupt
	write32(HW_PPCIRQMASK, (1<<30));
	clear32(HW_RESETS, 0x30);
	udelay(100);
	set32(HW_RESETS, 0x20);
	udelay(100);
	set32(HW_RESETS, 0x10);
	udelay(100000);
	set32(HW_EXICTRL, EXICTRL_ENABLE_EXI);
}

void powerpc_ipc(volatile ipc_request *req)
{
	switch (req->req) {
	case IPC_PPC_BOOT:
		if (req->args[0]) {
			// Enqueued from ARM side, do not invalidate mem nor ipc_post
			powerpc_boot_mem((u8 *) req->args[1], req->args[2]);
		} else {
			dc_invalidaterange((void *) req->args[1], req->args[2]);
			int res = powerpc_boot_mem((u8 *) req->args[1], req->args[2]);
			if (res)
				ipc_post(req->code, req->tag, 1, res);
		}

		break;

	case IPC_PPC_BOOT_FILE:
		if (req->args[0]) {
			// Enqueued from ARM side, do not invalidate mem nor ipc_post
			powerpc_boot_file((char *) req->args[1]);
		} else {
			dc_invalidaterange((void *) req->args[1],
								strnlen((char *) req->args[1], 256));
			int res = powerpc_boot_file((char *) req->args[1]);
			if (res)
				ipc_post(req->code, req->tag, 1, res);
		}

		break;

	default:
		gecko_printf("IPC: unknown SLOW PPC request %04X\n", req->req);
	}
}

