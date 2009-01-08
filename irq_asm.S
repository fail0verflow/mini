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
	bic	r1, r0, #0x1f
	orr	r1, #0x12
	msr cpsr, r1
	
	@ Setup interrupt stack
	ldr	sp, =__stack_addr
	add	sp, r4

	@ Restore mode
	msr cpsr, r0
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
	msr		cpsr, r1
	bx		lr

irq_restore:
	mrs		r1, cpsr
	bic		r1, r1, #(CPSR_IRQDIS|CPSR_FIQDIS)
	orr		r1, r1, r0
	msr		cpsr, r1
	bx		lr
