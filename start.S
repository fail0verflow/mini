.arm

.extern _main
.extern __got_start
.extern __got_end
.extern __bss_start
.extern __bss_end
.extern __stack_addr
.globl _start
.globl debug_output
.globl panic
.globl delay
.globl read32, write32
.globl read16, write16
.globl read8, write8
.globl getcpuid

.section .init

_start:
	@ Get real address of _start
	sub	r4, pc, #8
	@ Subtract offset to get the address that we were loaded at
	ldr	r0, =_start
	sub	r4, r4, r0
	@ Output 0x42 to the debug port
	mov	r0, #0x42
	bl	debug_output
	
	@ Set up a stack
	ldr	sp, =__stack_addr
	add	sp, r4
	
	@ perform boot2v3 memory controller poke
	bl memctrl_do_sub_sub_poke

	@ Output 0x43 to the debug port
	mov	r0, #0x43
	bl	debug_output
	
	@ relocate the GOT entries
	ldr	r1, =__got_start
	add	r1, r4
	ldr	r2, =__got_end
	add	r2, r4
got_loop:
	@ check for the end
	cmp	r1, r2
	beq	done_got
	@ read the GOT entry
	ldr	r3, [r1]
	@ add our base address
	add	r3, r4
	str	r3, [r1]
	@ move on
	add	r1, r1, #4
	b	got_loop

done_got:
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
	mov	r0, #0x44
	bl	debug_output
	@ take the plunge
	mov	r0, r4
	bl	_main
	@ _main returned! Go to whatever address it returned...
	mov	pc, r0

memctrl_do_sub_sub_poke:
	stmdb	sp!, {lr}
	ldr		r0, =0x163	@ reg_address
	mov		r1, #0x4C	@ address
	bl		memctrl_sub_poke
	ldr		r0, =0x163	@ read address back (flush?)
	bl		memctrl_sub_peek
	ldr		r0, =0x162	@ reg_data
	mov		r1, #1		@ data
	bl		memctrl_sub_poke
	ldmia	sp!, {pc}

memctrl_sub_poke:
	ldr		r2, =0xD8B4000
	strh	r0, [r2, #0x74] @ reg_address <= address
	ldrh	r0, [r2, #0x74] @ read reg_address back
	strh	r1, [r2, #0x76] @ reg_data <= data
	mov		pc, lr

memctrl_sub_peek:
	ldr		r2, =0xD8B4000
	strh	r0, [r2, #0x74] @ reg_address <= address
	ldrh	r0, [r2, #0x74] @ read reg_address back
	ldrh	r0, [r2, #0x76] @ data <= reg_data
	mov		pc, lr

.pool

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
	mov	pc, lr

panic:
	mov	r4, r0
_panic:
	mov	r0, r4
	bl	debug_output
	ldr	r0, =6175000
	bl	delay
	mov	r0, #0x00
	bl	debug_output
	ldr	r0, =6175000
	bl	delay
	b	_panic

@ the speed of this seems to decrease wildly with certain (non-)alignments
@ probably some prefetch buffer / cache / DRAM junk
	.balign 64
delay:
	cmp	r0, #0
	moveq	pc, lr
1:
	subs	r0, r0, #1
	bne	1b
	mov	pc, lr

read32:
	ldr	r0, [r0]
	mov	pc, lr

write32:
	str	r1,	[r0]
	mov	pc, lr

read16:
	ldrh	r0, [r0]
	mov	pc, lr

write16:
	strh	r1,	[r0]
	mov	pc, lr

read8:
	ldrb	r0, [r0]
	mov	pc, lr

write8:
	strb	r1,	[r0]
	mov	pc, lr

getcpuid:
	mrc     p15, 0, r0, c0, c0 
	mov pc, lr
	