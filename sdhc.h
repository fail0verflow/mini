/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	sd host controller driver
	
Copyright (C) 2008, 2009 	Sven Peter <svenpeter@gmail.com>

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

/* parts based on:
 *   * "SD Host Controller driver based on the SD Host Controller Standard" copyright (c) 2006 Uwe Stuehler <uwe@openbsd.org>
 *   * Simplified SD Host Controller Standard
 */

#ifndef __SDHC_H__
#define __SDHC_H__

#include "types.h"
#include "ipc.h"

#define	SDHC_ENOCARD	-0x1001
#define	SDHC_ESTRANGE	-0x1002
#define	SDHC_EOVERFLOW	-0x1003
#define	SDHC_ETIMEDOUT	-0x1004
#define	SDHC_EINVAL	-0x1005
#define	SDHC_EIO	-0x1006

/*struct {
	unsigned int		manfid;
	char			prod_name[8];
	unsigned int		serial;
	unsigned short		oemid;
	unsigned short		year;
	unsigned char		hwrev;
	unsigned char		fwrev;
	unsigned char		month;
} __attribute((packed) cid_str;
*/

typedef struct {
	unsigned char		dummy;
	unsigned char		mid;
	char		oid[2];
	char		pnm[5];
	unsigned char		prv;
	unsigned int		psn;
	unsigned short		mdt;
} __attribute__((packed)) cid_str;

typedef struct
{
	u32 reg_base;

	u8 is_sdhc;
	u8 is_selected;
	u8 is_mounted;

	u16 rca;
	u32 ocr;

	cid_str cid;
	u32 timeout;
	u32 num_sectors;
} sdhci_t;

int sd_init(sdhci_t *sdhci, int slot);
int sd_reset(sdhci_t *sdhci);
int sd_inserted(sdhci_t *sdhci);
int sd_protected(sdhci_t *sdhci);
int sd_mount(sdhci_t *sdhci);
int sd_cmd(sdhci_t *sdhci, u32 cmd, u32 rsp_type, u32 arg, u32 blk_cnt, void *buffer, u32 *response, u8 rlen);
int sd_select(sdhci_t *sdhci);
int sd_read(sdhci_t *sdhci, u32 start_block, u32 blk_cnt, void *buffer);
int sd_write(sdhci_t *sdhci, u32 start_block, u32 blk_cnt, const void *buffer);

void sd_irq(void);
void sd_initialize();

void sd_ipc(volatile ipc_request *req);

u8 __sd_read8(u32 addr);
u16 __sd_read16(u32 addr);
u32 __sd_read32(u32 addr);

#endif
