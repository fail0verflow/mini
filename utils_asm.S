/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	random utilities

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

.globl memcpy32
.globl memcpy16
.globl memcpy8
.globl memset32
.globl memset16
.globl memset8

.text

memcpy32:
	bics	r2, #3
	bxeq	lr
1:	ldr		r3, [r1],#4
	str		r3, [r0],#4
	subs	r2, #4
	bne		1b
	bx		lr

memset32:
	bics	r2, #3
	bxeq	lr
1:	str		r1, [r0],#4
	subs	r2, #4
	bne		1b
	bx		lr

memcpy16:
	bics	r2, #1
	bxeq	lr
1:	ldrh	r3, [r1],#2
	strh	r3, [r0],#2
	subs	r2, #2
	bne		1b
	bx		lr

memset16:
	bics	r2, #1
	bxeq	lr
1:	strh	r1, [r0],#2
	subs	r2, #2
	bne		1b
	bx		lr

memcpy8:
	cmp		r2, #0
	bxeq	lr
1:	ldrb	r3, [r1],#1
	strb	r3, [r0],#1
	subs	r2, #1
	bne		1b
	bx		lr

memset8:
	cmp		r2, #0
	bxeq	lr
1:	strb	r1, [r0],#1
	subs	r2, #1
	bne		1b
	bx		lr

