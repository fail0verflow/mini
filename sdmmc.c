/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	SD/MMC interface

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

#include "bsdtypes.h"
#include "sdhcreg.h"
#include "sdhcvar.h"
#include "sdmmcchip.h"
#include "sdmmcreg.h"
#include "sdmmcvar.h"
#include "sdmmc.h"
#include "gecko.h"
#include "string.h"
#include "utils.h"

#define SDMMC_DEBUG	1

#ifdef SDMMC_DEBUG
static int sdmmcdebug = 5;
#define DPRINTF(n,s)	do { if ((n) <= sdmmcdebug) gecko_printf s; } while (0)
#else
#define DPRINTF(n,s)	do {} while(0)
#endif

struct sdmmc_card {
	struct sdmmc_chip_functions *functions;
	sdmmc_chipset_handle_t handle;
	char name[255];
	int no;
	int inserted;
	int sdhc_blockmode;
};

static struct sdmmc_card cards[SDHC_MAX_HOSTS];
static int n_cards = 0;

static inline int sdmmc_host_reset(struct sdmmc_card *card)
{
	return sdmmc_chip_host_reset(card->functions, card->handle);
}
static inline int sdmmc_host_card_detect(struct sdmmc_card *card)
{
	return sdmmc_chip_card_detect(card->functions, card->handle);
#if 0
#define sdmmc_chip_host_maxblklen(tag, handle)				\
	((tag)->host_maxblklen((handle)))
#endif
}
static inline int sdmmc_host_ocr(struct sdmmc_card *card)
{
	return sdmmc_chip_host_ocr(card->functions, card->handle);
}
static inline int sdmmc_host_power(struct sdmmc_card *card, int ocr)
{
	return sdmmc_chip_bus_power(card->functions, card->handle, ocr);
}
static inline int sdmmc_host_clock(struct sdmmc_card *card, int freq)
{
	return sdmmc_chip_bus_clock(card->functions, card->handle, freq);
}
static inline void sdmmc_host_exec_command(struct sdmmc_card *card, struct
		sdmmc_command *cmd)
{
	sdmmc_chip_exec_command(card->functions, card->handle, cmd);
}

struct device *sdmmc_attach(struct sdmmc_chip_functions *functions,
		sdmmc_chipset_handle_t handle, const char *name, int no)
{
	struct sdmmc_card *c;

	if (n_cards >= SDHC_MAX_HOSTS) {
		gecko_printf("n_cards(%d) >= %d!\n", n_cards, SDHC_MAX_HOSTS);
		gecko_printf("starlet is soo going to crash very soon...\n");

		// HACK
		return (struct device *)-1;
	}

	c = &cards[n_cards++];
	memset(c, 0, sizeof(*c));

	c->functions = functions;
	c->handle = handle;
	c->no = no;
	strncpy(c->name, name, sizeof(c->name));

	DPRINTF(0, ("sdmmc: attached new SD/MMC card %d for host [%s:%d]\n",
				n_cards-1, c->name, c->no));

	sdmmc_host_reset(c);

	if (sdmmc_host_card_detect(c)) {
		DPRINTF(1, ("card is inserted. starting init sequence.\n"));
		sdmmc_needs_discover((struct device *)(n_cards-1));
	}

	// HACK
	return (struct device *)(n_cards-1);
}

void sdmmc_needs_discover(struct device *dev)
{
	int no = (int)dev;
	struct sdmmc_card *c = &cards[no];
	struct sdmmc_command cmd;
	u32 ocr;

	DPRINTF(0, ("sdmmc: card %d needs discovery.\n", no));

	if (!sdmmc_host_card_detect(c)) {
		DPRINTF(1, ("sdmmc: card %d (no longer?) inserted.\n", no));
		c->inserted = 0;
		return;
	}
	
	DPRINTF(1, ("sdmmc: enabling power for %d\n", no));
	if (sdmmc_host_power(c, MMC_OCR_3_2V_3_3V|MMC_OCR_3_3V_3_4V) != 0) {
		gecko_printf("sdmmc: powerup failed for card %d\n", no);
		goto out;
	}

	DPRINTF(1, ("sdmmc: enabling clock for %d\n", no));
	if (sdmmc_host_clock(c, SDMMC_DEFAULT_CLOCK) != 0) {
		gecko_printf("sdmmc: could not enable clock for card %d\n", no);
		goto out_power;
	}

	DPRINTF(1, ("sdmmc: sending GO_IDLE_STATE for %d\n", no));

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_GO_IDLE_STATE;
	cmd.c_flags = SCF_RSP_R0;
	sdmmc_host_exec_command(c, &cmd);

	if (cmd.c_error) {
		gecko_printf("sdmmc: GO_IDLE_STATE failed with %d for card %d\n",
				cmd.c_error, no);
		goto out_clock;
	}
	DPRINTF(2, ("sdmmc: GO_IDLE_STATE response for %d: %x\n", no,
				MMC_R1(cmd.c_resp)));

	DPRINTF(1, ("sdmmc: sending SEND_IF_COND for %d\n", no));

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = SD_SEND_IF_COND;
	cmd.c_arg = 0x1aa;
	cmd.c_flags = SCF_RSP_R7;
	sdmmc_host_exec_command(c, &cmd);

	gecko_printf("response: %08x %08x %08x %08x\n", cmd.c_resp[0],
			cmd.c_resp[1], cmd.c_resp[2], cmd.c_resp[3]);

	ocr = sdmmc_host_ocr(c);
	if (cmd.c_error || (cmd.c_resp[0] & 0xff) != 0xaa)
		ocr &= ~SD_OCR_SDHC_CAP;
	else
		ocr |= SD_OCR_SDHC_CAP;
	gecko_printf("-----> ocr: %x\n", ocr);
	DPRINTF(2, ("sdmmc: SEND_IF_COND ocr: %x\n", ocr));

	int tries;
	for (tries = 10; tries > 0; tries--) {
		udelay(10000);
	
		memset(&cmd, 0, sizeof(cmd));
		cmd.c_opcode = MMC_APP_CMD;
		cmd.c_arg = 0;
		cmd.c_flags = SCF_RSP_R1;
		sdmmc_host_exec_command(c, &cmd);

		if (cmd.c_error)
			continue;
	
		memset(&cmd, 0, sizeof(cmd));
		cmd.c_opcode = SD_APP_OP_COND;
		cmd.c_arg = ocr;
		cmd.c_flags = SCF_RSP_R3;
		sdmmc_host_exec_command(c, &cmd);
		if (cmd.c_error)
			continue;

		gecko_printf("response: %08x\n", cmd.c_resp[0]);
		if (ISSET(cmd.c_resp[0], MMC_OCR_MEM_READY))
			break;
	}
	if (!ISSET(cmd.c_resp[0], MMC_OCR_MEM_READY)) {
		gecko_printf("sdmmc: card %d failed to powerup.\n", no);
		goto out_power;
	}

	return;

out_clock:
out_power:
	sdmmc_host_power(c, 0);
out:
	return;

#if 0
//	struct sdmmc_task c_task;	/* task queue entry */
	u_int16_t	 c_opcode;	/* SD or MMC command index */
	u_int32_t	 c_arg;		/* SD/MMC command argument */
	sdmmc_response	 c_resp;	/* response buffer */
	void		*c_data;	/* buffer to send or read into */
	int		 c_datalen;	/* length of data buffer */
	int		 c_blklen;	/* block length */
	int		 c_flags;	/* see below */
#define SCF_ITSDONE	 0x0001		/* command is complete */
#define SCF_CMD(flags)	 ((flags) & 0x00f0)
#define SCF_CMD_AC	 0x0000
#define SCF_CMD_ADTC	 0x0010
#define SCF_CMD_BC	 0x0020
#define SCF_CMD_BCR	 0x0030
#define SCF_CMD_READ	 0x0040		/* read command (data expected) */
#define SCF_RSP_BSY	 0x0100
#define SCF_RSP_136	 0x0200
#define SCF_RSP_CRC	 0x0400
#define SCF_RSP_IDX	 0x0800
#define SCF_RSP_PRESENT	 0x1000
/* response types */
#define SCF_RSP_R0	 0 /* none */
#define SCF_RSP_R1	 (SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX)
#define SCF_RSP_R1B	 (SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX|SCF_RSP_BSY)
#define SCF_RSP_R2	 (SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_136)
#define SCF_RSP_R3	 (SCF_RSP_PRESENT)
#define SCF_RSP_R4	 (SCF_RSP_PRESENT)
#define SCF_RSP_R5	 (SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX)
#define SCF_RSP_R5B	 (SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX|SCF_RSP_BSY)
#define SCF_RSP_R6	 (SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX)
#define SCF_RSP_R7	 (SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX)
	int		 c_error;	/* errno value on completion */

	/* Host controller owned fields for data xfer in progress */
	int c_resid;			/* remaining I/O */
	u_char *c_buf;			/* remaining data */
#endif
}

