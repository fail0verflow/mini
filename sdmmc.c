/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	SD/MMC interface

Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "bsdtypes.h"
#include "sdhc.h"
#include "gecko.h"
#include "string.h"
#include "utils.h"
#include "memory.h"

//#define SDMMC_DEBUG

#ifdef SDMMC_DEBUG
static int sdmmcdebug = 0;
#define DPRINTF(n,s)	do { if ((n) <= sdmmcdebug) gecko_printf s; } while (0)
#else
#define DPRINTF(n,s)	do {} while(0)
#endif

struct sdmmc_card {
	sdmmc_chipset_handle_t handle;
	int inserted;
	int sdhc_blockmode;
	int selected;
	int new_card; // set to 1 everytime a new card is inserted

	u32 timeout;
	u32 num_sectors;
	u32 cid;
	u16 rca;
};

#ifdef LOADER
static struct sdmmc_card card;
#else
static struct sdmmc_card card MEM2_BSS;
#endif

void sdmmc_attach(sdmmc_chipset_handle_t handle)
{
	memset(&card, 0, sizeof(card));

	card.handle = handle;

	DPRINTF(0, ("sdmmc: attached new SD/MMC card\n"));

	sdhc_host_reset(card.handle);

	if (sdhc_card_detect(card.handle)) {
		DPRINTF(1, ("card is inserted. starting init sequence.\n"));
		sdmmc_needs_discover();
	}
}

void sdmmc_abort(void) {
	struct sdmmc_command cmd;
	gecko_printf("abortion kthx\n");
	
	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_STOP_TRANSMISSION;
	cmd.c_arg = 0;
	cmd.c_flags = SCF_RSP_R1B;
	sdhc_exec_command(card.handle, &cmd);
}

void sdmmc_needs_discover(void)
{
	struct sdmmc_command cmd;
	u32 ocr;

	DPRINTF(0, ("sdmmc: card needs discovery.\n"));
	sdhc_host_reset(card.handle);
	card.new_card = 1;

	if (!sdhc_card_detect(card.handle)) {
		DPRINTF(1, ("sdmmc: card (no longer?) inserted.\n"));
		card.inserted = 0;
		return;
	}
	
	DPRINTF(1, ("sdmmc: enabling power\n"));
	if (sdhc_bus_power(card.handle, 1) != 0) {
		gecko_printf("sdmmc: powerup failed for card\n");
		goto out;
	}

	DPRINTF(1, ("sdmmc: enabling clock\n"));
	if (sdhc_bus_clock(card.handle, SDMMC_DEFAULT_CLOCK) != 0) {
		gecko_printf("sdmmc: could not enable clock for card\n");
		goto out_power;
	}

	DPRINTF(1, ("sdmmc: sending GO_IDLE_STATE\n"));

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_GO_IDLE_STATE;
	cmd.c_flags = SCF_RSP_R0;
	sdhc_exec_command(card.handle, &cmd);

	if (cmd.c_error) {
		gecko_printf("sdmmc: GO_IDLE_STATE failed with %d\n", cmd.c_error);
		goto out_clock;
	}
	DPRINTF(2, ("sdmmc: GO_IDLE_STATE response: %x\n", MMC_R1(cmd.c_resp)));

	DPRINTF(1, ("sdmmc: sending SEND_IF_COND\n"));

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = SD_SEND_IF_COND;
	cmd.c_arg = 0x1aa;
	cmd.c_flags = SCF_RSP_R7;
	cmd.c_timeout = 100;
	sdhc_exec_command(card.handle, &cmd);

	ocr = card.handle->ocr;
	if (cmd.c_error || (cmd.c_resp[0] & 0xff) != 0xaa)
		ocr &= ~SD_OCR_SDHC_CAP;
	else
		ocr |= SD_OCR_SDHC_CAP;
	DPRINTF(2, ("sdmmc: SEND_IF_COND ocr: %x\n", ocr));

	int tries;
	for (tries = 100; tries > 0; tries--) {
		udelay(100000);
	
		memset(&cmd, 0, sizeof(cmd));
		cmd.c_opcode = MMC_APP_CMD;
		cmd.c_arg = 0;
		cmd.c_flags = SCF_RSP_R1;
		sdhc_exec_command(card.handle, &cmd);

		if (cmd.c_error)
			continue;
	
		memset(&cmd, 0, sizeof(cmd));
		cmd.c_opcode = SD_APP_OP_COND;
		cmd.c_arg = ocr;
		cmd.c_flags = SCF_RSP_R3;
		sdhc_exec_command(card.handle, &cmd);
		if (cmd.c_error)
			continue;

		DPRINTF(3, ("sdmmc: response for SEND_IF_COND: %08x\n",
					MMC_R1(cmd.c_resp)));
		if (ISSET(MMC_R1(cmd.c_resp), MMC_OCR_MEM_READY))
			break;
	}
	if (!ISSET(cmd.c_resp[0], MMC_OCR_MEM_READY)) {
		gecko_printf("sdmmc: card failed to powerup.\n");
		goto out_power;
	}

	if (ISSET(MMC_R1(cmd.c_resp), SD_OCR_SDHC_CAP))
		card.sdhc_blockmode = 1;
	else
		card.sdhc_blockmode = 0;
	DPRINTF(2, ("sdmmc: SDHC: %d\n", card.sdhc_blockmode));
	
	u8 *resp;
	DPRINTF(2, ("sdmmc: MMC_ALL_SEND_CID\n"));
	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_ALL_SEND_CID;
	cmd.c_arg = 0;
	cmd.c_flags = SCF_RSP_R2;
	sdhc_exec_command(card.handle, &cmd);
	if (cmd.c_error) {
		gecko_printf("sdmmc: MMC_ALL_SEND_CID failed with %d\n", cmd.c_error);
		goto out_clock;
	}

	card.cid = MMC_R1(cmd.c_resp);
	resp = (u8 *)cmd.c_resp;
	gecko_printf("CID: mid=%02x name='%c%c%c%c%c%c%c' prv=%d.%d psn=%02x%02x%02x%02x mdt=%d/%d\n", resp[14], 
		resp[13],resp[12],resp[11],resp[10],resp[9],resp[8],resp[7], resp[6], resp[5] >> 4, resp[5] & 0xf, 
		resp[4], resp[3], resp[2], resp[0] & 0xf, 2000 + (resp[0] >> 4));
		
	DPRINTF(2, ("sdmmc: SD_SEND_RELATIVE_ADDRESS\n"));
	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = SD_SEND_RELATIVE_ADDR;
	cmd.c_arg = 0;
	cmd.c_flags = SCF_RSP_R6;
	sdhc_exec_command(card.handle, &cmd);
	if (cmd.c_error) {
		gecko_printf("sdmmc: SD_SEND_RCA failed with %d\n", cmd.c_error);
		goto out_clock;
	}

	card.rca = MMC_R1(cmd.c_resp)>>16;
	DPRINTF(2, ("sdmmc: rca: %08x\n", card.rca));

	card.selected = 0;
	card.inserted = 1;

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_SEND_CSD;
	cmd.c_arg = ((u32)card.rca)<<16;
	cmd.c_flags = SCF_RSP_R2;
	sdhc_exec_command(card.handle, &cmd);
	if (cmd.c_error) {
		gecko_printf("sdmmc: MMC_SEND_CSD failed with %d\n", cmd.c_error);
		goto out_power;
	}

	resp = (u8 *)cmd.c_resp;

	int i;
	gecko_printf("csd: ");
	for(i=15; i>=0; i--) gecko_printf("%02x ", (u32) resp[i]);
	gecko_printf("\n");

	if (resp[13] == 0xe) { // sdhc
		unsigned int c_size = resp[7] << 16 | resp[6] << 8 | resp[5];
		gecko_printf("sdmmc: sdhc mode, c_size=%u, card size = %uk\n", c_size, (c_size + 1)* 512);
		card.timeout = 250 * 1000000; // spec says read timeout is 100ms and write/erase timeout is 250ms
		card.num_sectors = (c_size + 1) * 1024; // number of 512-byte sectors
	}
	else {
		unsigned int taac, nsac, read_bl_len, c_size, c_size_mult;
		taac = resp[13];
		nsac = resp[12];
		read_bl_len = resp[9] & 0xF;
		
		c_size = (resp[8] & 3) << 10;
		c_size |= (resp[7] << 2);
		c_size |= (resp[6] >> 6);
		c_size_mult = (resp[5] & 3) << 1;
		c_size_mult |= resp[4] >> 7;
		gecko_printf("taac=%u nsac=%u read_bl_len=%u c_size=%u c_size_mult=%u card size=%u bytes\n",
			taac, nsac, read_bl_len, c_size, c_size_mult, (c_size + 1) * (4 << c_size_mult) * (1 << read_bl_len));
		static const unsigned int time_unit[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000};
		static const unsigned int time_value[] = {1, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80}; // must div by 10
		card.timeout = time_unit[taac & 7] * time_value[(taac >> 3) & 0xf] / 10;
		gecko_printf("calculated timeout =  %uns\n", card.timeout);
		card.num_sectors = (c_size + 1) * (4 << c_size_mult) * (1 << read_bl_len) / 512;
	}

	sdmmc_select();
	DPRINTF(2, ("sdmmc: MMC_SET_BLOCKLEN\n"));
	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_SET_BLOCKLEN;
	cmd.c_arg = SDMMC_DEFAULT_BLOCKLEN;
	cmd.c_flags = SCF_RSP_R1;
	sdhc_exec_command(card.handle, &cmd);
	if (cmd.c_error) {
		gecko_printf("sdmmc: MMC_SET_BLOCKLEN failed with %d\n", cmd.c_error);
		card.inserted = card.selected = 0;
		goto out_clock;
	}
	return;

out_clock:
	sdhc_bus_clock(card.handle, SDMMC_SDCLK_OFF);

out_power:
	sdhc_bus_power(card.handle, 0);
out:
	return;
}


int sdmmc_select(void)
{
	struct sdmmc_command cmd;

	DPRINTF(2, ("sdmmc: MMC_SELECT_CARD\n"));
	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_SELECT_CARD;
	cmd.c_arg = ((u32)card.rca)<<16;
	cmd.c_flags = SCF_RSP_R1B;
	sdhc_exec_command(card.handle, &cmd);
	gecko_printf("%s: resp=%x\n", __FUNCTION__, MMC_R1(cmd.c_resp));
//	sdhc_dump_regs(card.handle);
	
//	gecko_printf("present state = %x\n", HREAD4(hp, SDHC_PRESENT_STATE));
	if (cmd.c_error) {
		gecko_printf("sdmmc: MMC_SELECT card failed with %d.\n", cmd.c_error);
		return -1;
	}

	card.selected = 1;
	return 0;
}

int sdmmc_check_card(void)
{
	if (card.inserted == 0)
		return SDMMC_NO_CARD;

	if (card.new_card == 1)
		return SDMMC_NEW_CARD;

	return SDMMC_INSERTED;
}

int sdmmc_ack_card(void)
{
	if (card.new_card == 1) {
		card.new_card = 0;
		return 0;
	}

	return -1;
}

int sdmmc_read(u32 blk_start, u32 blk_count, void *data)
{
	struct sdmmc_command cmd;

//	gecko_printf("%s(%u, %u, %p)\n", __FUNCTION__, blk_start, blk_count, data);
	if (card.inserted == 0) {
		gecko_printf("sdmmc: READ: no card inserted.\n");
		return -1;
	}

	if (card.selected == 0) {
		if (sdmmc_select() < 0) {
			gecko_printf("sdmmc: READ: cannot select card.\n");
			return -1;
		}
	}

	if (card.new_card == 1) {
		gecko_printf("sdmmc: new card inserted but not acknowledged yet.\n");
		return -1;
	}

	DPRINTF(2, ("sdmmc: MMC_READ_BLOCK_MULTIPLE\n"));
	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_READ_BLOCK_MULTIPLE;
	if (card.sdhc_blockmode)
		cmd.c_arg = blk_start;
	else
		cmd.c_arg = blk_start * SDMMC_DEFAULT_BLOCKLEN;
	cmd.c_data = data;
	cmd.c_datalen = blk_count * SDMMC_DEFAULT_BLOCKLEN;
	cmd.c_blklen = SDMMC_DEFAULT_BLOCKLEN;
	cmd.c_flags = SCF_RSP_R1 | SCF_CMD_READ;
	sdhc_exec_command(card.handle, &cmd);

	if (cmd.c_error) {
		gecko_printf("sdmmc: MMC_READ_BLOCK_MULTIPLE failed with %d\n", cmd.c_error);
		return -1;
	}
	DPRINTF(2, ("sdmmc: MMC_READ_BLOCK_MULTIPLE done\n"));

	return 0;
}

#ifndef LOADER
int sdmmc_write(u32 blk_start, u32 blk_count, void *data)
{
	struct sdmmc_command cmd;

	if (card.inserted == 0) {
		gecko_printf("sdmmc: READ: no card inserted.\n");
		return -1;
	}

	if (card.selected == 0) {
		if (sdmmc_select() < 0) {
			gecko_printf("sdmmc: READ: cannot select card.\n");
			return -1;
		}
	}

	if (card.new_card == 1) {
		gecko_printf("sdmmc: new card inserted but not acknowledged yet.\n");
		return -1;
	}

	DPRINTF(2, ("sdmmc: MMC_WRITE_BLOCK_MULTIPLE\n"));
	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_WRITE_BLOCK_MULTIPLE;
	if (card.sdhc_blockmode)
		cmd.c_arg = blk_start;
	else
		cmd.c_arg = blk_start * SDMMC_DEFAULT_BLOCKLEN;
	cmd.c_data = data;
	cmd.c_datalen = blk_count * SDMMC_DEFAULT_BLOCKLEN;
	cmd.c_blklen = SDMMC_DEFAULT_BLOCKLEN;
	cmd.c_flags = SCF_RSP_R1;
	sdhc_exec_command(card.handle, &cmd);

	if (cmd.c_error) {
		gecko_printf("sdmmc: MMC_READ_BLOCK_MULTIPLE failed with %d\n", cmd.c_error);
		return -1;
	}
	DPRINTF(2, ("sdmmc: MMC_WRITE_BLOCK_MULTIPLE done\n"));

	return 0;
}

int sdmmc_get_sectors(void)
{
	if (card.inserted == 0) {
		gecko_printf("sdmmc: READ: no card inserted.\n");
		return -1;
	}

	if (card.new_card == 1) {
		gecko_printf("sdmmc: new card inserted but not acknowledged yet.\n");
		return -1;
	}

//	sdhc_error(sdhci->reg_base, "num sectors = %u", sdhci->num_sectors);
	
	return card.num_sectors;
}
#endif

#ifdef CAN_HAZ_IPC
void sdmmc_ipc(volatile ipc_request *req)
{
	int ret;
	switch (req->req) {
	case IPC_SDMMC_ACK:
		ret = sdmmc_ack_card();
		ipc_post(req->code, req->tag, 1, ret);
		break;
	case IPC_SDMMC_READ:
		ret = sdmmc_read(req->args[0], req->args[1], (void *)req->args[2]);
		dc_flushrange((void *)req->args[2],
				req->args[1]*SDMMC_DEFAULT_BLOCKLEN);
		ipc_post(req->code, req->tag, 1, ret);
		break;
	case IPC_SDMMC_WRITE:
		dc_invalidaterange((void *)req->args[2],
				req->args[1]*SDMMC_DEFAULT_BLOCKLEN);
		ret = sdmmc_write(req->args[0],	req->args[1], (void *)req->args[2]);
		ipc_post(req->code, req->tag, 1, ret);
		break;
	case IPC_SDMMC_STATE:
		ipc_post(req->code, req->tag, 1,
				sdmmc_check_card());
		break;
	case IPC_SDMMC_SIZE:
		ipc_post(req->code, req->tag, 1,
				sdmmc_get_sectors());
		break;
	}
}
#endif
