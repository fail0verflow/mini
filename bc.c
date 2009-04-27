/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	BC support code

Copyright (C) 2009	Haxx Enterprises <bushing@gmail.com>

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
#include "memory.h"
#include "powerpc.h"
#include "hollywood.h"
#include "utils.h"
#include "start.h"
#include "gecko.h"
#if 0
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
#endif
void toggle_ppc1_ppc2(void)
{
	u32 d = read32(HW_RESETS);
	write32(HW_RESETS, d & ~0x30);
	udelay(15);
	write32(HW_RESETS, d & ~0x10);
}

void clear_resets_i_l(void)
{	
}

void clear_184_438e(void)
{	
}

void hw_bus_freq_craps(void)
{	
}

void call_init_dram(int r0, int r1)
{	
}

void set_hwresets_i_l(void)
{
}

void clear_hwresets_i_l(void)
{
}

void set_184_438e(void)
{
}

void twiddle_dram_regs(int r0, int r1)
{
}

void bc_main(void) {
	toggle_ppc1_ppc2();
	clear_hwresets_i_l();
	clear_184_438e();
	hw_bus_freq_craps();
	call_init_dram(1,0);
	set_hwresets_i_l();
	set_184_438e();
	twiddle_dram_regs(1,1);
	write32(HW_PPCIRQFLAG, 1 << 15);
	udelay(200);
	mem_setswap_bc();
}
#if 0
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
	default:
		gecko_printf("IPC: unknown SLOW PPC request %04X\n", req->req);
	}
}

#endif
