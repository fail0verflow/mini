/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	glue layer for FatFs

Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>
Copyright (C) 2008, 2009	Haxx Enterprises <bushing@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "diskio.h"
#include "string.h"
#include "sdmmc.h"

#ifndef MEM2_BSS
#define MEM2_BSS
#endif

static u8 buffer[512] MEM2_BSS ALIGNED(32);

// Initialize a Drive
DSTATUS disk_initialize (BYTE drv) {
	if (sdmmc_check_card() == SDMMC_NO_CARD)
		return STA_NOINIT;

	sdmmc_ack_card();
	return disk_status(drv);
}

// Return Disk Status
DSTATUS disk_status (BYTE drv) {
	(void)drv;
	if (sdmmc_check_card() == SDMMC_INSERTED)
		return 0;
	else
		return STA_NODISK;
}

// Read Sector(s)
DRESULT disk_read (BYTE drv, BYTE *buff, DWORD sector, BYTE count) {
	int i;
	(void)drv;
	for (i = 0; i < count; i++) {
		if (sdmmc_read(sector+i, 1, buffer) != 0)
			return RES_ERROR;
		memcpy(buff + i * 512, buffer, 512);
	}

	return RES_OK;
}

// Write Sector(s)
#if _READONLY == 0
DRESULT disk_write (BYTE drv, const BYTE *buff, DWORD sector, BYTE count) {
	int i;

	for (i = 0; i < count; i++) {
		memcpy(buffer, buff + i * 512, 512);

		if(sdmmc_write(sector + i, 1, buffer) != 0)
			return RES_ERROR;
	}

	return RES_OK;
}
#endif /* _READONLY */

#if _USE_IOCTL == 1
DRESULT disk_ioctl (BYTE drv, BYTE ctrl, void *buff) {
	if (ctrl == CTRL_SYNC)
		return RES_OK;

	return RES_PARERR;
}
#endif /* _USE_IOCTL */
