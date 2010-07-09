/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	boot2 chainloader

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>
Copyright (C) 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "types.h"
#include "elf.h"
#include "nand.h"
#include "memory.h"
#include "crypto.h"
#include "string.h"
#include "gecko.h"
#include "powerpc.h"
#include "utils.h"
#include "panic.h"
#include "boot2.h"

static u8 boot2[0x80000] MEM2_BSS ALIGNED(64);
static u8 boot2_key[32] MEM2_BSS ALIGNED(32);
static u8 boot2_iv[32] MEM2_BSS ALIGNED(32);
static u8 sector_buf[PAGE_SIZE] MEM2_BSS ALIGNED(64);
static u8 ecc_buf[ECC_BUFFER_ALLOC] MEM2_BSS ALIGNED(128);
static u8 boot2_initialized = 0;
static u8 boot2_copy;
static u8 pages_read;
static u8 *page_ptr;

typedef struct {
	u32 len;
	u32 data_offset;
	u32 certs_len;
	u32 tik_len;
	u32 tmd_len;
	u32 padding[3];
} boot2header;

typedef struct {
	u64 signature;
	u32 generation;
	u8 blocks[0x40];
} __attribute__((packed)) boot2blockmap;

static boot2blockmap good_blockmap MEM2_BSS;

#define BLOCKMAP_SIGNATURE 0x26f29a401ee684cfULL

#define BOOT2_START 1
#define BOOT2_END 7

static u8 boot2_blocks[BOOT2_END - BOOT2_START + 1];
static u32 valid_blocks;

static tmd_t tmd MEM2_BSS;
static tik_t tik MEM2_BSS;
static u8 *boot2_content;
static u32 boot2_content_size;

// find two equal valid blockmaps from a set of three, return one of them
static int find_valid_map(const boot2blockmap *maps)
{
	if(maps[0].signature == BLOCKMAP_SIGNATURE) {
		if(!memcmp(&maps[0],&maps[1],sizeof(boot2blockmap)))
			return 0;
		if(!memcmp(&maps[0],&maps[2],sizeof(boot2blockmap)))
			return 0;
	}
	if(maps[1].signature == BLOCKMAP_SIGNATURE) {
		if(!memcmp(&maps[1],&maps[2],sizeof(boot2blockmap)))
			return 1;
	}
	return -1;
}

// translate a page offset into boot2 to a real NAND page number using blockmap
static inline u32 boot2_page_translate(u32 page)
{
	u32 subpage = page % BLOCK_SIZE;
	u32 block = page / BLOCK_SIZE;

	return boot2_blocks[block] * BLOCK_SIZE + subpage;
}

// read boot2 up to the specified number of bytes (aligned to the next page)
static int read_to(u32 bytes)
{
	if(bytes > (valid_blocks * BLOCK_SIZE * PAGE_SIZE)) {
		gecko_printf("tried to read %d boot2 bytes (%d pages), but only %d blocks (%d pages) are valid!\n",
			bytes, (bytes+(PAGE_SIZE-1)) / PAGE_SIZE, valid_blocks, valid_blocks * BLOCK_SIZE);
		return -1;
	}
	while(bytes > ((u32)pages_read * PAGE_SIZE)) {
		u32 page = boot2_page_translate(pages_read);
		nand_read_page(page, page_ptr, ecc_buf);
		nand_wait();
		if(nand_correct(page, page_ptr, ecc_buf) < 0) {
			gecko_printf("boot2 page %d (NAND 0x%x) is uncorrectable\n", pages_read, page);
			return -1;
		}
		page_ptr += PAGE_SIZE;
		pages_read++;
	}
	return 0;
}

int boot2_load(int copy)
{
	boot2blockmap *maps = (boot2blockmap*)sector_buf;
	u32 block;
	u32 page;
	int mapno;
	u32 found = 0;
	boot2header *hdr;
	u8 iv[16];

	boot2_content = NULL;
	boot2_content_size = 0;
	pages_read = 0;
	memset(&good_blockmap, 0, sizeof(boot2blockmap));
	valid_blocks = 0;

	// find the best blockmap
	for(block=BOOT2_START; block<=BOOT2_END; block++) {
		page = (block+1)*BLOCK_SIZE - 1;
		nand_read_page(page, sector_buf, ecc_buf);
		nand_wait();
		// boot1 doesn't actually do this, but it's probably a good idea to try to correct 1-bit errors anyway
		if(nand_correct(page, sector_buf, ecc_buf) < 0) {
			gecko_printf("boot2 map candidate page 0x%x is uncorrectable, trying anyway\n", page);
		}
		mapno = find_valid_map(maps);
		if(mapno >= 0) {
			gecko_printf("found valid boot2 blockmap at page 0x%x, submap %d, generation %d\n",
				page, mapno, maps[mapno].generation);
			if(maps[mapno].generation >= good_blockmap.generation) {
				memcpy(&good_blockmap, &maps[mapno], sizeof(boot2blockmap));
				found = 1;
			}
		}
	}

	if(!found) {
		gecko_printf("no valid boot2 blockmap found!\n");
		return -1;
	}

	// traverse the blockmap and make a list of the actual boot2 blocks, in order
	if(copy == 0) {
		for(block=BOOT2_START; block<=BOOT2_END; block++) {
			if(good_blockmap.blocks[block] == 0x00) {
				boot2_blocks[valid_blocks++] = block;
			}
		}
	} else if(copy == 1) {
		for(block=BOOT2_END; block>=BOOT2_START; block--) {
			if(good_blockmap.blocks[block] == 0x00) {
				boot2_blocks[valid_blocks++] = block;
			}
		}
	} else {
		gecko_printf("invalid boot2 copy %d\n", copy);
		return -1;
	}

	gecko_printf("boot2 blocks:");
	for(block=0; block<valid_blocks; block++)
		gecko_printf(" %02x", boot2_blocks[block]);
	gecko_printf("\n");

	// read boot2 header
	page_ptr = boot2;
	if(read_to(sizeof(boot2header)) < 0) {
		gecko_printf("error while reading boot2 header");
		return -1;
	}

	hdr = (boot2header *)boot2;

	if(hdr->len != sizeof(boot2header)) {
		gecko_printf("invalid boot2 header size 0x%x\n", hdr->len);
		return -1;
	}
	if(hdr->tmd_len != sizeof(tmd_t)) {
		gecko_printf("boot2 tmd size mismatch: expected 0x%x, got 0x%x (more than one content?)\n", sizeof(tmd_t), hdr->tmd_len);
		return -1;
	}
	if(hdr->tik_len != sizeof(tik_t)) {
		gecko_printf("boot2 tik size mismatch: expected 0x%x, got 0x%x\n", sizeof(tik_t), hdr->tik_len);
		return -1;
	}

	// read tmd, tik, certs
	if(read_to(hdr->data_offset) < 0) {
		gecko_printf("error while reading boot2 certs/tmd/ticket");
		return -1;
	}

	memcpy(&tik, &boot2[hdr->len + hdr->certs_len], sizeof(tik_t));
	memcpy(&tmd, &boot2[hdr->len + hdr->certs_len + hdr->tik_len], sizeof(tmd_t));

	memset(iv, 0, 16);
	memcpy(iv, &tik.titleid, 8);

	aes_reset();
	aes_set_iv(iv);
	aes_set_key(otp.common_key);
	memcpy(boot2_key, &tik.cipher_title_key, 16);

	aes_decrypt(boot2_key, boot2_key, 1, 0);

	memset(boot2_iv, 0, 16);
	memcpy(boot2_iv, &tmd.contents.index, 2); //just zero anyway...

#if defined(CAN_HAZ_USBGECKO) && !defined(NDEBUG)
	u32 *kp = (u32*)boot2_key;
	gecko_printf("boot2 title key: %08x%08x%08x%08x\n", kp[0], kp[1], kp[2], kp[3]);
#endif

	boot2_content_size = (tmd.contents.size + 15) & ~15;
	gecko_printf("boot2 content size: 0x%x (padded: 0x%x)\n",
		(u32)tmd.contents.size, boot2_content_size);

	// read content
	if(read_to(hdr->data_offset + boot2_content_size) < 0) {
		gecko_printf("error while reading boot2 content");
		return -1;
	}

	boot2_content = &boot2[hdr->data_offset];

	boot2_copy = copy;
	gecko_printf("boot2 copy %d loaded to %p\n", copy, boot2);
	return 0;
}

void boot2_init(void) {
	boot2_copy = -1;
	boot2_initialized = 0;
	if(boot2_load(0) < 0) {
		gecko_printf("failed to load boot2 copy 0, trying copy 1...\n");
		if(boot2_load(1) < 0) {
			gecko_printf("failed to load boot2 copy 1!\n");
			return;
		}
	}

	// boot2 content flush would flush entire cache anyway so just do it all
	dc_flushall();
	boot2_initialized = 1;
}

static u32 match[] = {
	0xBC024708,
	1,
	2,
};

static u32 patch[] = {
	0xBC024708,
	0, // tid hi
	0, // tid low
};

static u32 boot2_patch(ioshdr *hdr) {
	u32 i, num_matches = 0;
	u8 *ptr = (u8 *) hdr + hdr->hdrsize + hdr->loadersize;

	for (i = 0; i < hdr->elfsize; i += 1) {
		if (memcmp(ptr+i, match, sizeof(match)) == 0) {
			num_matches++;
			memcpy(ptr+i, patch, sizeof(patch));
			gecko_printf("patched data @%08x\n", (u32)ptr+i);
		}
	}

	return num_matches;
}

u32 boot2_run(u32 tid_hi, u32 tid_lo) {
	u32 num_matches;
	ioshdr *hdr;
	
	gecko_printf("booting boot2 with title %08x-%08x\n", tid_hi, tid_lo);
	mem_protect(1, (void *)0x11000000, (void *)0x13FFFFFF);

	aes_reset();
	aes_set_iv(boot2_iv);
	aes_set_key(boot2_key);
	aes_decrypt(boot2_content, (void *)0x11000000, boot2_content_size / 16, 0);

	hdr = (ioshdr *) 0x11000000;

	if ((tid_hi != match[1]) || (tid_lo != match[2])) {
		patch[1] = tid_hi;
		patch[2] = tid_lo;

		num_matches = boot2_patch(hdr);

		if (num_matches != 1) {
			gecko_printf("Wrong number of patches (matched %d times, expected 1), panicking\n", num_matches);
			panic2(0, PANIC_PATCHFAIL);
		}
	}
	
	hdr->argument = 0x42;

	u32 vector = 0x11000000 + hdr->hdrsize;
	gecko_printf("boot2 is at 0x%08x\n", vector);
	return vector;
}

u32 boot2_ipc(volatile ipc_request *req)
{
	u32 vector = 0;

	switch (req->req) {
		case IPC_BOOT2_RUN:
			if(boot2_initialized) {
				// post first so that the memory protection doesn't kill IPC for the PowerPC
				ipc_post(req->code, req->tag, 1, boot2_copy);
				ipc_flush();
				vector = boot2_run((u32)req->args[0], (u32)req->args[1]);
			} else {
				ipc_post(req->code, req->tag, 1, -1);
			}
			break;

		case IPC_BOOT2_TMD:
			if (boot2_initialized)
				ipc_post(req->code, req->tag, 1, &tmd);
			else
				ipc_post(req->code, req->tag, 1, -1);

			break;

		default:
			gecko_printf("IPC: unknown SLOW BOOT2 request %04X\n", req->req);
	}

	return vector;
}

