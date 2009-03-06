#include "irq.h"
#include "hollywood.h"
#include "gecko.h"
#include "utils.h"
#include "ipc.h"
#include "memory.h"
#include "panic.h"

const char *exceptions[] = {
	"RESET", "UNDEFINED INSTR", "SWI", "INSTR ABORT", "DATA ABORT", "RESERVED", "IRQ", "FIQ", "(unknown exception type)"
};

const char *aborts[] = {
	"UNDEFINED",
	"Alignment",
	"UNDEFINED",
	"Alignment",
	"UNDEFINED",
	"Translation",
	"UNDEFINED",
	"Translation",
	"External abort",
	"Domain",
	"External abort",
	"Domain",
	"External abort on translation (first level)",
	"Permission",
	"External abort on translation (second level)",
	"Permission"
};

u8 domvalid[] = {0,0,0,0,0,0,0,1,0,1,0,1,0,1,1,1};

void exc_setup_stack(void);

void exception_initialize(void)
{
	exc_setup_stack();
	u32 cr = get_cr();
	cr |= 0x2; // Data alignment fault checking enable
	set_cr(cr);
}

void exc_handler(u32 type, u32 spsr, u32 *regs)
{
	if (type > 8) type = 8;
	gecko_printf("\nException %d (%s):\n", type, exceptions[type]);

	u32 pc, fsr;

	switch(type) {
		case 7: // FIQ
		case 3: // INSTR ABORT
			pc = regs[15] - 4;
			break;
		case 4: // DATA ABORT
			pc = regs[15] - 8;
			break;
		default:
			pc = regs[15];
			break;
	}

	gecko_printf("Registers (%p):\n", regs);
	gecko_printf("  R0-R3: %08x %08x %08x %08x\n", regs[0], regs[1], regs[2], regs[3]);
	gecko_printf("  R4-R7: %08x %08x %08x %08x\n", regs[4], regs[5], regs[6], regs[7]);
	gecko_printf(" R8-R11: %08x %08x %08x %08x\n", regs[8], regs[9], regs[10], regs[11]);
	gecko_printf("R12-R15: %08x %08x %08x %08x\n", regs[12], regs[13], regs[14], pc);

	gecko_printf("SPSR: %08x\n", spsr);
	gecko_printf("CPSR: %08x\n", get_cpsr());
	gecko_printf("CR:   %08x\n", get_cr());
	gecko_printf("TTBR: %08x\n", get_ttbr());
	gecko_printf("DACR: %08x\n", get_dacr());

	switch (type) {
		case 1: // undefined instruction
			gecko_printf("Undefined instruction @ %08x:\n%08x %08x *%08x* %08x %08x\n", 
						pc, read32(pc-8), read32(pc-4), read32(pc), read32(pc+4), read32(pc+8));
			break;
		case 3: // INSTR ABORT
		case 4: // DATA ABORT 
			if(type == 3)
				fsr = get_ifsr();
			else
				fsr = get_dfsr();
			gecko_printf("Abort type: %s\n", aborts[fsr&0xf]);
			if(domvalid[fsr&0xf])
				gecko_printf("Domain: %d\n", (fsr>>4)&0xf);
			if(type == 4)
				gecko_printf("Address: 0x%08x\n", get_far());
		break;
		default: break;
	}

	panic2(0, PANIC_EXCEPTION);
}
