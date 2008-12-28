#include "types.h"
#include "powerpc.h"
#include "hollywood.h"
#include "utils.h"
#include "start.h"
#include "gecko.h"


const u32 stub_default[0x10] = {
	0x3c600000,
	0x60633400,
	0x7c7a03a6,
	0x38600000,
	0x7c7b03a6,
	0x4c000064,
	0,
};

void powerpc_upload_stub(const u32 *stub, u32 len)
{
	u32 i;

	set32(HW_EXICTRL, EXICTRL_ENABLE_EXI);

	if(stub == NULL || len == 0)
	{
		stub = stub_default;
		len = sizeof(stub_default) / sizeof(u32);
	}

	for(i = 0; i < len; i++)
		write32(EXI_BOOT_BASE + 4*i, stub[i]);
	
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
