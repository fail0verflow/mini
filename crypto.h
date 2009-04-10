/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	crypto support

Copyright (C) 2008, 2009	Haxx Enterprises
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
#ifndef __CRYPTO_H__
#define __CRYPTO_H__	1

#include "types.h"
#include "ipc.h"

typedef struct
{
	u8 boot1_hash[20];
	u8 common_key[16];
	u32 ng_id;
	union {
		 struct {
			 u8 ng_priv[30];
			 u8 _wtf1[18];
		 };
		 struct {
			 u8 _wtf2[28];
			 u8 nand_hmac[20];
		 };
	};
	u8 nand_key[16];
	u8 rng_key[16];
	u32 unk1;
	u32 unk2; // 0x00000007
} __attribute__((packed)) otp_t;

typedef struct
{
	u8 boot2version;
	u8 unknown1;
	u8 unknown2;
	u8 pad;
	u32 update_tag;
	u16 checksum;
} __attribute__((packed)) eep_ctr_t;

typedef struct
{
	union {
		struct {
			u32 dunno0; // 0x2 = MS
			u32 dunno1; // 0x1 = CA
			u32 ng_key_id;
			u8 ng_sig[60];
			eep_ctr_t counters[2];
			u8 fill[0x18];
			u8 korean_key[16];
		};
		u8 data[256];
	};
} __attribute__((packed)) seeprom_t;

extern otp_t otp;

void crypto_read_otp();
void crypto_ipc(volatile ipc_request *req);

void crypto_initialize();

void aes_reset(void);
void aes_set_iv(u8 *iv);
void aes_empty_iv();
void aes_set_key(u8 *key);
void aes_decrypt(u8 *src, u8 *dst, u32 blocks, u8 keep_iv);
void aes_ipc(volatile ipc_request *req);

#endif
