#ifndef __IRQ_H__
#define __IRQ_H__

#define IRQ_TIMER	0
#define IRQ_NAND	1
#define IRQ_AES		2
#define IRQ_SHA1	3
#define IRQ_EHCI	4
#define IRQ_OHCI0	5
#define IRQ_OHCI1	6
#define IRQ_SDHC	7
#define IRQ_WIFI	8
#define IRQ_GPIO1B	10
#define IRQ_GPIO1	11
#define IRQ_RESET	17
#define IRQ_PPCIPC	30
#define IRQ_IPC		31

#define IRQF_TIMER	(1<<IRQ_TIMER)
#define IRQF_NAND	(1<<IRQ_NAND)
#define IRQF_AES	(1<<IRQ_AES)
#define IRQF_GPIO1B	(1<<IRQ_GPIO1B)
#define IRQF_GPIO1	(1<<IRQ_GPIO1)
#define IRQF_RESET	(1<<IRQ_RESET)
#define IRQF_IPC	(1<<IRQ_IPC)

#define IRQF_ALL	( \
	IRQF_TIMER|IRQF_NAND|IRQF_GPIO1B|IRQF_GPIO1| \
	IRQF_RESET|IRQF_IPC|IRQF_AES \
	)

#define CPSR_IRQDIS 0x80
#define CPSR_FIQDIS 0x40

#ifndef _LANGUAGE_ASSEMBLY
#include "types.h"

void irq_initialize(void);
void irq_shutdown(void);

void irq_enable(u32 irq);
void irq_disable(u32 irq);

u32 irq_kill(void);
void irq_restore(u32 cookie);

static inline void irq_wait(void)
{
	u32 data = 0;
	__asm__ volatile ( "mcr\tp15, 0, %0, c7, c0, 4" : : "r" (data) );
}

#endif

#endif
