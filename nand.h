/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	memory management, MMU, caches, and flushing

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>

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
#ifndef __NAND_H__
#define __NAND_H__

#include "types.h"
#include "ipc.h"

#define PAGE_SIZE			2048
#define PAGE_SPARE_SIZE		64
#define ECC_BUFFER_SIZE		(PAGE_SPARE_SIZE+16)
#define ECC_BUFFER_ALLOC	(PAGE_SPARE_SIZE+32)
#define BLOCK_SIZE			64
#define NAND_MAX_PAGE		0x40000

void nand_irq(void);

void nand_send_command(u32 command, u32 bitmask, u32 flags, u32 num_bytes);
int nand_reset(void);
void nand_get_id(u8 *);
void nand_get_status(u8 *);
void nand_read_page(u32 pageno, void *data, void *ecc);
void nand_write_page(u32 pageno, void *data, void *ecc);
void nand_erase_block(u32 pageno);
void nand_wait(void);

void nand_read_cluster(u32 clusterno, void *data);

#define NAND_ECC_OK 0
#define NAND_ECC_CORRECTED 1
#define NAND_ECC_UNCORRECTABLE -1

int nand_correct(u32 pageno, void *data, void *ecc);

void nand_initialize();

void nand_ipc(volatile ipc_request *req);

#endif

