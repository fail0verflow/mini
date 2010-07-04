/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	boot2 chainloader

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __BOOT2_H__
#define __BOOT2_H__

#include "ipc.h"

typedef struct {
	u32 cid;
	u16 index;
	u16 type;
	u64 size;
	u8 hash[20];
} __attribute__((packed)) tmd_content_t;

typedef struct {
	u32 type;
	u8 sig[256];
	u8 fill[60];
} __attribute__((packed)) sig_rsa2048_t;

typedef struct {
	sig_rsa2048_t signature;
	char issuer[0x40];
	u8 version;
	u8 ca_crl_version;
	u8 signer_crl_version;
	u8 fill2;
	u64 sys_version;
	u64 title_id;
	u32 title_type;
	u16 group_id;
	u16 zero;
	u16 region;
	u8 ratings[16];
	u8 reserved[42];
	u32 access_rights;
	u16 title_version;
	u16 num_contents;
	u16 boot_index;
	u16 fill3;
	tmd_content_t contents;
} __attribute__((packed)) tmd_t;

typedef struct _tik {
	sig_rsa2048_t signature;
	char issuer[0x40];
	u8 fill[63];
	u8 cipher_title_key[16];
	u8 fill2;
	u64 ticketid;
	u32 devicetype;
	u64 titleid;
	u16 access_mask;
	u8 reserved[0x3c];
	u8 cidx_mask[0x40];
	u16 padding;
	u32 limits[16];
} __attribute__((packed)) tik_t;

u32 boot2_run(u32 tid_hi, u32 tid_lo);
void boot2_init();
u32 boot2_ipc(volatile ipc_request *req);

#endif

