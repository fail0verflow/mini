#include "types.h"
#include "nand.h"
#include "memory.h"
#include "crypto.h"
#include "string.h"
#include "gecko.h"
#include "powerpc.h"

static u8 boot2[256 << 11] MEM2_BSS __attribute__((aligned(64)));
static u8 key[32] MEM2_BSS __attribute__((aligned(64)));
static u8 ecc[64] __attribute__((aligned(64)));
static u8 boot2_initialized = 0;
extern void *vector;

struct wadheader {
	u32 len;
	u32 type;
	u32 certs_len;
	u32 tik_len;
	u32 tmd_len;
	u32 reserved; // ??
	u32 data_len; // ??
	u32 footer_len; // ??
};
typedef struct {
	u32 hdrsize;
	u32 loadersize;
	u32 elfsize;
	u32 argument;
} ioshdr;

void boot2_init() {
	int i;
	void *ptr = boot2;
	u8 *tikptr = NULL;
	u8 *tmdptr = NULL;
	u8 *cntptr = NULL;
	u8 iv[16];
	struct wadheader *hdr = (struct wadheader *)boot2;
	u32 datalen;	

	for (i = 0x40; i < 0x140; i++, ptr += 2048) {
		nand_read_page(i, ptr, ecc);
		__nand_wait();
	}

	if (hdr->len != sizeof(struct wadheader))
		return;

	tikptr = boot2 + hdr->len + hdr->certs_len;
	tmdptr = tikptr + hdr->tik_len;
	cntptr = ALIGN_FORWARD(tmdptr + hdr->tmd_len, 0x40);

	memset(iv, 0, 16);
	memcpy(iv, tikptr + 0x1dc, 8);

	aes_reset();
	aes_set_iv(iv);
	aes_set_key(otp.common_key);
	memcpy(key, tikptr+0x1bf, 16);
	gecko_puts("flushing now...\n");
	dc_flushrange(key, 32);
	gecko_puts("dc_flush done.\n");
	aes_decrypt(key, key, 1, 0);
	dc_invalidaterange(key, 32);

	memcpy(&datalen, tmdptr+0x1e4+8+4, 4);
	memset(iv, 0, 16);
	memcpy(iv, tmdptr + 0x1e4 + 4, 2);

	aes_reset();
	aes_set_iv(iv);
	aes_set_key(key);
	dc_flushrange(cntptr, ALIGN_FORWARD(datalen, 16));
	aes_decrypt(cntptr, cntptr, ALIGN_FORWARD(datalen, 16)/16, 0);
	dc_invalidaterange(cntptr, ALIGN_FORWARD(datalen, 16));

	memcpy(boot2, cntptr, datalen);
	boot2_initialized = 1;
}

static u32 match[] = {
	0xF7FFFFB8,
	0xBC024708,
	1,
	2,
};

static u32 patch[] = {
	0xF7FFFFB8,
	0xBC024708,
	0x10001,
	0x48415858,
};

int boot2_run(u32 tid_hi, u32 tid_lo) {
	void *ptr;
	int i;
	ioshdr *hdr = (ioshdr *)boot2;

	if (boot2_initialized != 1)
		return 0;

	patch[2] = tid_hi;
	patch[3] = tid_lo;

	powerpc_hang();
	memcpy((void *)0x11000000, boot2, sizeof boot2);
	ptr = (void *)0x11000000 + hdr->hdrsize + hdr->loadersize;
	for (i = 0; i < sizeof(boot2); i += 1) {
		if (memcmp(ptr+i, match, sizeof(match)) == 0) {
			memcpy(ptr+i, patch, sizeof(patch));
			gecko_printf("patched data @%08x\n", ptr+i);
		}
	}

	hdr = (ioshdr *)0x11000000;
	hdr->argument = 0x42;
	vector = (void *)0x11000000 + hdr->hdrsize;
	gecko_printf("boot2 is at %p\n", vector);
	return 1;
}
