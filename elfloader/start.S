/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	ELF loader: system startup

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>

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
.arm

.extern _main
.extern __got_start
.extern __got_end
.extern __bss_start
.extern __bss_end
.extern __stack_addr
.extern delay
.globl _start
.globl debug_output

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
	mov	r1, r0
	mov	r0, r4
	mov	pc, r1

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
