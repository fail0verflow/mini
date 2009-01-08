#include "irq.h"
#include "hollywood.h"
#include "gecko.h"
#include "utils.h"

void irq_setup_stack(void);

void irq_initialize(void)
{
	irq_setup_stack();
	write32(HW_IRQENABLE, 0);
	write32(HW_IRQFLAG, 0xffffffff);
	irq_restore(CPSR_FIQDIS);

	//???
	write32(HW_IRQENABLE+0x04, 0);
	write32(HW_IRQENABLE+0x20, 0);
}

void irq_shutdown(void)
{
	write32(HW_IRQENABLE, 0);
	write32(HW_IRQFLAG, 0xffffffff);
	irq_kill();
}

void irq_handler(void)
{
	u32 enabled = read32(HW_IRQENABLE);
	u32 flags = read32(HW_IRQFLAG);
	
	gecko_printf("In IRQ handler: 0x%08x 0x%08x 0x%08x\n", enabled, flags, flags & enabled);

	flags = flags & enabled;

	if(flags & IRQF_TIMER) {
		gecko_printf("IRQ: timer\n");
		gecko_printf("Timer: %08x\n", read32(HW_TIMER));
		gecko_printf("Alarm: %08x\n", read32(HW_ALARM));
		write32(HW_ALARM, 0); // shut it up
		write32(HW_IRQFLAG, IRQF_TIMER);
	}
	if(flags & IRQF_NAND) {
		gecko_printf("IRQ: NAND\n");
		write32(NAND_CMD, 0x7fffffff); // shut it up
		write32(HW_IRQFLAG, IRQF_NAND);
	}
	if(flags & IRQF_GPIO1B) {
		gecko_printf("IRQ: GPIO1B\n");
		write32(HW_GPIO1BINTFLAG, 0xFFFFFF); // shut it up
		write32(HW_IRQFLAG, IRQF_GPIO1B);
	}
	if(flags & IRQF_GPIO1) {
		gecko_printf("IRQ: GPIO1\n");
		write32(HW_GPIO1INTFLAG, 0xFFFFFF); // shut it up
		write32(HW_IRQFLAG, IRQF_GPIO1);
	}
	if(flags & IRQF_RESET) {
		gecko_printf("IRQ: RESET\n");
		write32(HW_IRQFLAG, IRQF_RESET);
	}
	if(flags & IRQF_IPC) {
		gecko_printf("IRQ: IPC\n");
		write32(HW_IRQFLAG, IRQF_IPC);
	}
	flags &= ~IRQF_ALL;
	if(flags) {
		gecko_printf("IRQ: unknown 0x%08x\n");
		write32(HW_IRQFLAG, flags);
	}
}

void irq_enable(u32 irq)
{
	set32(HW_IRQENABLE, 1<<irq);
}

void irq_disable(u32 irq)
{
	clear32(HW_IRQENABLE, 1<<irq);
}
