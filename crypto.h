#ifndef __CRYPTO_H__
#define __CRYPTO_H__	1

#include "types.h"

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
	u32 unk2;
} __attribute__((packed)) otp_t;

typedef struct
{
	union {
		struct {
			u32 dunno0; // 0x2 = MS
			u32 dunno1; // 0x1 = CA
			u32 ng_key_id;
			u8 ng_sig[60];
			u8 fill[0x2c];
			u8 korean_key[16];
		};
		u8 data[512];
	};
} __attribute__((packed)) seeprom_t;

extern otp_t otp;

void crypto_read_otp();

void crypto_initialize();

void aes_reset(void);
void aes_set_iv(u8 *iv);
void aes_empty_iv();
void aes_set_key(u8 *key);
void aes_decrypt(u8 *src, u8 *dst, u32 blocks, u8 keep_iv);

#endif
