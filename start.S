.arm

.extern _main
.extern __got_start
.extern __got_end
.extern __bss_start
.extern __bss_end
.extern __bss2_start
.extern __bss2_end
.extern __stack_addr
.globl _start
.globl debug_output

.extern v_irq

.section .init

_vectors:
_start:
	ldr	pc, =v_reset
	ldr	pc, =v_undf
	ldr	pc, =v_swi
	ldr	pc, =v_instr_abrt
	ldr	pc, =v_data_abrt
	ldr	pc, =v_reserved
	ldr	pc, =v_irq
	ldr	pc, =v_fiq

.pool

v_reset:
	@ Get loader base from ELF loader
	mov	r4, r0

	@ Output 0x42 to the debug port
	mov	r0, #0x42
	bl	debug_output
	
	@ Set up a stack
	ldr	sp, =__stack_addr
	add	sp, r4

	@ clear BSS
	ldr	r1, =__bss_start
	add	r1, r4
	ldr	r2, =__bss_end
	add	r2, r4
	mov	r3, #0
bss_loop:
	@ check for the end
	cmp	r1, r2
	beq	done_bss
	@ clear the word and move on
	str	r3, [r1]
	add	r1, r1, #4
	b	bss_loop

done_bss:
	@ clear BSS2
	ldr	r1, =__bss2_start
	add	r1, r4
	ldr	r2, =__bss2_end
	add	r2, r4
	mov	r3, #0
bss2_loop:
	@ check for the end
	cmp	r1, r2
	beq	done_bss2
	@ clear the word and move on
	str	r3, [r1]
	add	r1, r1, #4
	b	bss2_loop

done_bss2:
	mov	r0, #0x84
	bl	debug_output
	@ take the plunge
	mov	r0, r4
	bl	_main
	@ _main returned! Go to whatever address it returned...
	mov	pc, r0

.pool

v_undf:
	b v_undf

v_swi:
	b v_swi

v_instr_abrt:
	b v_instr_abrt

v_data_abrt:
	b v_data_abrt

v_reserved:
	b v_reserved

#v_irq:
#	b v_irq

v_fiq:
	b v_fiq

debug_output:
	@ load address of port
	mov	r3, #0xd800000
	@ load old value
	ldr	r2, [r3, #0xe0]
	@ clear debug byte
	bic	r2, r2, #0xFF0000
	@ insert new value
	and	r0, r0, #0xFF
	orr	r2, r2, r0, LSL #16
	@ store back
	str	r2, [r3, #0xe0]
	bx	lr

.pool
