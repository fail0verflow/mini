/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	SD/MMC interface

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
#ifndef __SDMMC_H__
#define __SDMMC_H__

#include "bsdtypes.h"
#include "sdmmcchip.h"
#include "sdmmcvar.h"
#ifdef CAN_HAZ_IPC
#include "ipc.h"
#endif

#define SDMMC_DEFAULT_CLOCK		25000
#define SDMMC_DEFAULT_BLOCKLEN		  512

#define SDMMC_NO_CARD			    1
#define SDMMC_NEW_CARD			    2
#define SDMMC_INSERTED			    3

// HACK
#define SDMMC_DEFAULT_DEVICE	((struct device *)0)

struct device *sdmmc_attach(struct sdmmc_chip_functions *functions,
		sdmmc_chipset_handle_t handle, const char *name, int no);
void sdmmc_needs_discover(struct device *dev);
int sdmmc_select(struct device *dev);
int sdmmc_check_card(struct device *dev);
void sdmmc_ack_card(struct device *dev);
int sdmmc_read(struct device *dev, u32 blk_start, u32 blk_count, void *data);
#ifdef CAN_HAZ_IPC
void sdmmc_ipc(volatile ipc_request *req);
#endif
#endif
