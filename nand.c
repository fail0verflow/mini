/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	low-level NAND support

Copyright (C) 2008, 2009	Haxx Enterprises <bushing@gmail.com>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>
Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "hollywood.h"
#include "nand.h"
#include "utils.h"
#include "string.h"
#include "start.h"
#include "memory.h"
#include "crypto.h"
#include "irq.h"
#include "ipc.h"
#include "gecko.h"
#include "types.h"

// #define	NAND_DEBUG	1
#define NAND_SUPPORT_WRITE 1
#define NAND_SUPPORT_ERASE 1

#ifdef NAND_DEBUG
#	include "gecko.h"
#	define	NAND_debug(f, arg...) gecko_printf("NAND: " f, ##arg);
#else
#	define	NAND_debug(f, arg...)
#endif

#define NAND_RESET      0xff
#define NAND_CHIPID     0x90
#define NAND_GETSTATUS  0x70
#define NAND_ERASE_PRE  0x60
#define NAND_ERASE_POST 0xd0
#define NAND_READ_PRE   0x00
#define NAND_READ_POST  0x30
#define NAND_WRITE_PRE  0x80
#define NAND_WRITE_POST 0x10

#define NAND_BUSY_MASK  0x80000000
#define NAND_ERROR      0x20000000

#define NAND_FLAGS_IRQ 	0x40000000
#define NAND_FLAGS_WAIT 0x8000
#define NAND_FLAGS_WR	0x4000
#define NAND_FLAGS_RD	0x2000
#define NAND_FLAGS_ECC	0x1000

static ipc_request current_request;

static u8 ipc_data[PAGE_SIZE] MEM2_BSS ALIGNED(32);
static u8 ipc_ecc[ECC_BUFFER_ALLOC] MEM2_BSS ALIGNED(128); //128 alignment REQUIRED

static volatile int irq_flag;
static u32 last_page_read = 0;
static u32 nand_min_page = 0x200; // default to protecting boot1+boot2

void nand_irq(void)
{
	int code, tag, err = 0;
	if(read32(NAND_CMD) & NAND_ERROR) {
		gecko_printf("NAND: Error on IRQ\n");
		err = -1;
	}
	ahb_flush_from(AHB_NAND);
	ahb_flush_to(AHB_STARLET);
	if (current_request.code != 0) {
		switch (current_request.req) {
			case IPC_NAND_GETID:
				memcpy32((void*)current_request.args[0], ipc_data, 0x40);
				dc_flushrange((void*)current_request.args[0], 0x40);
				break;
			case IPC_NAND_STATUS:
				memcpy32((void*)current_request.args[0], ipc_data, 0x40);
				dc_flushrange((void*)current_request.args[0], 0x40);
				break;
			case IPC_NAND_READ:
				err = nand_correct(last_page_read, ipc_data, ipc_ecc);

				if (current_request.args[1] != 0xFFFFFFFF) {
					memcpy32((void*)current_request.args[1], ipc_data, PAGE_SIZE);
					dc_flushrange((void*)current_request.args[1], PAGE_SIZE);
				}
				if (current_request.args[2] != 0xFFFFFFFF) {
					memcpy32((void*)current_request.args[2], ipc_ecc, PAGE_SPARE_SIZE);
					dc_flushrange((void*)current_request.args[2], PAGE_SPARE_SIZE);
				}
				break;
			case IPC_NAND_ERASE:
				// no action needed upon erase completion
				break;
			case IPC_NAND_WRITE:
				// no action needed upon write completion
				break;
			default:
				gecko_printf("Got IRQ for unknown NAND req %d\n", current_request.req);
		}
		code = current_request.code;
		tag = current_request.tag;
		current_request.code = 0;
		ipc_post(code, tag, 1, err);
	}
	irq_flag = 1;
}

inline void __nand_wait(void) {
	while(read32(NAND_CMD) & NAND_BUSY_MASK);
	if(read32(NAND_CMD) & NAND_ERROR)
		gecko_printf("NAND: Error on wait\n");
	ahb_flush_from(AHB_NAND);
	ahb_flush_to(AHB_STARLET);
}

void nand_send_command(u32 command, u32 bitmask, u32 flags, u32 num_bytes) {
	u32 cmd = NAND_BUSY_MASK | (bitmask << 24) | (command << 16) | flags | num_bytes;

	NAND_debug("nand_send_command(%x, %x, %x, %x) -> %x\n",
		command, bitmask, flags, num_bytes, cmd);

	write32(NAND_CMD, 0x7fffffff);
	write32(NAND_CMD, 0);
	write32(NAND_CMD, cmd);
}

void __nand_set_address(s32 page_off, s32 pageno) {
	if (page_off != -1) write32(NAND_ADDR0, page_off);
	if (pageno != -1)   write32(NAND_ADDR1, pageno);
}

void __nand_setup_dma(u8 *data, u8 *spare) {
	if (((s32)data) != -1) {
		write32(NAND_DATA, dma_addr(data));
	}
	if (((s32)spare) != -1) {
		u32 addr = dma_addr(spare);
		if(addr & 0x7f)
			gecko_printf("NAND: Spare buffer 0x%08x is not aligned, data will be corrupted\n", addr);
		write32(NAND_ECC, addr);
	}
}

int nand_reset(void) {
	NAND_debug("nand_reset()\n");
// IOS actually uses NAND_FLAGS_IRQ | NAND_FLAGS_WAIT here
	nand_send_command(NAND_RESET, 0, NAND_FLAGS_WAIT, 0);
	__nand_wait();
// enable NAND controller
	write32(NAND_CONF, 0x08000000);
// set configuration parameters for 512MB flash chips
	write32(NAND_CONF, 0x4b3e0e7f);
	return 0;
}

void nand_get_id(u8 *idbuf) {
	irq_flag = 0;
	__nand_set_address(0,0);

	dc_invalidaterange(idbuf, 0x40);

	__nand_setup_dma(idbuf, (u8 *)-1);
	nand_send_command(NAND_CHIPID, 1, NAND_FLAGS_IRQ | NAND_FLAGS_RD, 0x40);
}

void nand_get_status(u8 *status_buf) {
	irq_flag = 0;
	status_buf[0]=0;

	dc_invalidaterange(status_buf, 0x40);

	__nand_setup_dma(status_buf, (u8 *)-1);
	nand_send_command(NAND_GETSTATUS, 0, NAND_FLAGS_IRQ | NAND_FLAGS_RD, 0x40);
}

void nand_read_page(u32 pageno, void *data, void *ecc) {
	irq_flag = 0;
	last_page_read = pageno;  // needed for error reporting
	__nand_set_address(0, pageno);
	nand_send_command(NAND_READ_PRE, 0x1f, 0, 0);

	if (((s32)data) != -1) dc_invalidaterange(data, PAGE_SIZE);
	if (((s32)ecc) != -1)  dc_invalidaterange(ecc, ECC_BUFFER_SIZE);

	__nand_wait();
	__nand_setup_dma(data, ecc);
	nand_send_command(NAND_READ_POST, 0, NAND_FLAGS_IRQ | NAND_FLAGS_WAIT | NAND_FLAGS_RD | NAND_FLAGS_ECC, 0x840);
}

void nand_wait(void) {
// power-saving IRQ wait
	while(!irq_flag) {
		u32 cookie = irq_kill();
		if(!irq_flag)
			irq_wait();
		irq_restore(cookie);
	}
}

#ifdef NAND_SUPPORT_WRITE
void nand_write_page(u32 pageno, void *data, void *ecc) {
	irq_flag = 0;
	NAND_debug("nand_write_page(%u, %p, %p)\n", pageno, data, ecc);

// this is a safety check to prevent you from accidentally wiping out boot1 or boot2.
	if ((pageno < nand_min_page) || (pageno >= NAND_MAX_PAGE)) {
		gecko_printf("Error: nand_write to page %d forbidden\n", pageno);
		return;
	}
	if (((s32)data) != -1) dc_flushrange(data, PAGE_SIZE);
	if (((s32)ecc) != -1)  dc_flushrange(ecc, PAGE_SPARE_SIZE);
	ahb_flush_to(AHB_NAND);
	__nand_set_address(0, pageno);
	__nand_setup_dma(data, ecc);
	nand_send_command(NAND_WRITE_PRE, 0x1f, NAND_FLAGS_WR, 0x840);
	__nand_wait();
	nand_send_command(NAND_WRITE_POST, 0, NAND_FLAGS_IRQ | NAND_FLAGS_WAIT, 0);
}
#endif

#ifdef NAND_SUPPORT_ERASE
void nand_erase_block(u32 pageno) {
	irq_flag = 0;
	NAND_debug("nand_erase_block(%d)\n", pageno);

// this is a safety check to prevent you from accidentally wiping out boot1 or boot2.
	if ((pageno < nand_min_page) || (pageno >= NAND_MAX_PAGE)) {
		gecko_printf("Error: nand_erase to page %d forbidden\n", pageno);
		return;
	}
	__nand_set_address(0, pageno);
	nand_send_command(NAND_ERASE_PRE, 0x1c, 0, 0);
	__nand_wait();
	nand_send_command(NAND_ERASE_POST, 0, NAND_FLAGS_IRQ | NAND_FLAGS_WAIT, 0);
	NAND_debug("nand_erase_block(%d) done\n", pageno);
}
#endif

void nand_initialize(void)
{
	current_request.code = 0;
	nand_reset();
	irq_enable(IRQ_NAND);
}

int nand_correct(u32 pageno, void *data, void *ecc)
{
	(void) pageno;

	u8 *dp = (u8*)data;
	u32 *ecc_read = (u32*)((u8*)ecc+0x30);
	u32 *ecc_calc = (u32*)((u8*)ecc+0x40);
	int i;
	int uncorrectable = 0;
	int corrected = 0;
	
	for(i=0;i<4;i++) {
		u32 syndrome = *ecc_read ^ *ecc_calc; //calculate ECC syncrome
		// don't try to correct unformatted pages (all FF)
		if ((*ecc_read != 0xFFFFFFFF) && syndrome) {
			if(!((syndrome-1)&syndrome)) {
				// single-bit error in ECC
				corrected++;
			} else {
				// byteswap and extract odd and even halves
				u16 even = (syndrome >> 24) | ((syndrome >> 8) & 0xf00);
				u16 odd = ((syndrome << 8) & 0xf00) | ((syndrome >> 8) & 0x0ff);
				if((even ^ odd) != 0xfff) {
					// oops, can't fix this one
					uncorrectable++;
				} else {
					// fix the bad bit
					dp[odd >> 3] ^= 1<<(odd&7);
					corrected++;
				}
			}
		}
		dp += 0x200;
		ecc_read++;
		ecc_calc++;
	}
	if(uncorrectable || corrected)
		gecko_printf("ECC stats for NAND page 0x%x: %d uncorrectable, %d corrected\n", pageno, uncorrectable, corrected);
	if(uncorrectable)
		return NAND_ECC_UNCORRECTABLE;
	if(corrected)
		return NAND_ECC_CORRECTED;
	return NAND_ECC_OK;
}

void nand_ipc(volatile ipc_request *req)
{
	u32 new_min_page = 0x200;
	if (current_request.code != 0) {
		gecko_printf("NAND: previous IPC request is not done yet.");
		ipc_post(req->code, req->tag, 1, -1);
		return;
	}
	switch (req->req) {
		case IPC_NAND_RESET:
			nand_reset();
			ipc_post(req->code, req->tag, 0);
			break;

		case IPC_NAND_GETID:
			current_request = *req;
			nand_get_id(ipc_data);
			break;

		case IPC_NAND_STATUS:
			current_request = *req;
			nand_get_status(ipc_data);
			break;

		case IPC_NAND_READ:
			current_request = *req;
			nand_read_page(req->args[0], ipc_data, ipc_ecc);
			break;
#ifdef NAND_SUPPORT_WRITE
		case IPC_NAND_WRITE:
			current_request = *req;
			dc_invalidaterange((void*)req->args[1], PAGE_SIZE);
			dc_invalidaterange((void*)req->args[2], PAGE_SPARE_SIZE);
			memcpy(ipc_data, (void*)req->args[1], PAGE_SIZE);
			memcpy(ipc_ecc, (void*)req->args[2], PAGE_SPARE_SIZE);
			nand_write_page(req->args[0], ipc_data, ipc_ecc);
			break;
#endif
#ifdef NAND_SUPPORT_ERASE
		case IPC_NAND_ERASE:
			current_request = *req;
			nand_erase_block(req->args[0]);
			break;
#endif
/* This is only here to support the truly brave or stupid who are using hardware hacks to reflash
   boot1/boot2 onto blank or corrupted NAND flash chips.  Best practices dictate that you should
   query minpage (and make sure it is the value you expect -- usually 0x200) before writing to NAND.
   If you call SETMINPAGE, you MUST then call GETMINPAGE to check that it actually succeeded, do your
   writes, and then as soon as possible call SETMINPAGE(0x200) to restore the default minimum page. */
		case IPC_NAND_SETMINPAGE:
			new_min_page = req->args[0];
			if (new_min_page > 0x200) {
				gecko_printf("Ignoring strange NAND_SETMINPAGE request: %u\n", new_min_page);
				break;
			}
			gecko_printf("WARNING: setting minimum allowed NAND page to %u\n", new_min_page);
			nand_min_page = new_min_page;
			ipc_post(req->code, req->tag, 0);
			break;
		case IPC_NAND_GETMINPAGE:
			ipc_post(req->code, req->tag, 1, nand_min_page);
			break;
		default:
			gecko_printf("IPC: unknown SLOW NAND request %04x\n",
					req->req);
	}
}

