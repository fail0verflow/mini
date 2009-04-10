/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	memory management, MMU, caches, and flushing

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

.globl _dc_inval_entries
.globl _dc_flush_entries
.globl _dc_flush
.globl _dc_inval
.globl _ic_inval
.globl _drain_write_buffer
.globl _tlb_inval

.text

_dc_inval_entries:
	mcr		p15, 0, r0, c7, c6, 1
	add		r0, #0x20
	subs	r1, #1
	bne		_dc_inval_entries
	bx		lr

_dc_flush_entries:
	mcr		p15, 0, r0, c7, c10, 1
	add		r0, #0x20
	subs	r1, #1
	bne		_dc_flush_entries
	bx		lr

_dc_flush:
	mrc		p15, 0, pc, c7, c10, 3
	bne		_dc_flush
	bx		lr

_dc_inval:
	mov		r0, #0
	mcr		p15, 0, r0, c7, c6, 0
	bx		lr

_ic_inval:
	mov		r0, #0
	mcr		p15, 0, r0, c7, c5, 0
	bx		lr

_drain_write_buffer:
	mov		r0, #0
	mcr		p15, 0, r0, c7, c10, 4
	bx		lr

_tlb_inval:
	mov		r0, #0
	mcr		p15, 0, r0, c8, c7
	bx		lr
