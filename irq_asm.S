#include "hollywood.h"
#include "irq.h"

.globl v_irq
.globl irq_setup_stack
.globl irq_kill
.globl irq_restore
.extern __irqstack_addr
.extern irq_handler

irq_setup_stack:
	@ Switch to IRQ mode
	mrs	r0, cpsr
	msr cpsr_c, #0xd2
	
	@ Setup interrupt stack
	ldr	sp, =__irqstack_addr

	@ Restore mode
	msr cpsr_c, r0
	bx	lr

v_irq:
	push	{r0-r3, r9, r12, lr}

	blx		irq_handler

	pop		{r0-r3, r9, r12, lr}
	subs	pc, lr, #4

irq_kill:
	mrs		r1, cpsr
	and		r0, r1, #(CPSR_IRQDIS|CPSR_FIQDIS)
	orr		r1, r1, #(CPSR_IRQDIS|CPSR_FIQDIS)
	msr		cpsr_c, r1
	bx		lr

irq_restore:
	mrs		r1, cpsr
	bic		r1, r1, #(CPSR_IRQDIS|CPSR_FIQDIS)
	orr		r1, r1, r0
	msr		cpsr_c, r1
	bx		lr
