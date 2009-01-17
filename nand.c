#include "hollywood.h"
#include "nand.h"
#include "utils.h"
#include "string.h"
#include "start.h"
#include "memory.h"
#include "crypto.h"
#include "irq.h"

//#define	NAND_DEBUG	1
#undef NAND_SUPPORT_WRITE
#undef NAND_SUPPORT_ERASE

#ifdef NAND_DEBUG
#	include "gecko.h"
#	define	NAND_debug(f, arg...) gecko_printf("NAND: " f "\n", ##arg);
#else
#	define	NAND_debug(f, arg...) 
#endif

#define STACK_ALIGN(type, name, cnt, alignment)         \
u8 _al__##name[((sizeof(type)*(cnt)) + (alignment) + \
(((sizeof(type)*(cnt))%(alignment)) > 0 ? ((alignment) - \
((sizeof(type)*(cnt))%(alignment))) : 0))]; \
type *name = (type*)(((u32)(_al__##name)) + ((alignment) - (( \
(u32)(_al__##name))&((alignment)-1))))


#define NAND_RESET 0xff
#define NAND_CHIPID 0x90
#define NAND_GETSTATUS 0x70
#define NAND_ERASE_PRE 0x60
#define NAND_ERASE_POST 0xd0
#define NAND_READ_PRE 0x00
#define NAND_READ_POST 0x30
#define NAND_WRITE_PRE 0x80
#define NAND_WRITE_POST 0x10

#define NAND_BUSY_MASK 0x80000000

#define	PAGE_SIZE			2048
#define PAGE_SPARE_SIZE		64

static int irq = 0;

void nand_irq(void)
{
	irq = 1;
}


static inline u32 __nand_read32(u32 addr)
{
	return read32(addr);
}

inline void __nand_write32(u32 addr, u32 data)
{
	write32(addr, data);
}

inline void __nand_wait(void) {
	while(__nand_read32(NAND_CMD) & NAND_BUSY_MASK);
}

void nand_send_command(u32 command, u32 bitmask, u32 flags, u32 num_bytes) {
	u32 cmd = NAND_BUSY_MASK | (bitmask << 24) | (command << 16) | flags | num_bytes;
	
	NAND_debug("nand_send_command(%x, %x, %x, %x) -> %x\n",
		command, bitmask, flags, num_bytes, cmd);
			
	__nand_write32(NAND_CMD, 0x7fffffff);
	__nand_write32(NAND_CMD, 0);
	__nand_write32(NAND_CMD, cmd);
	__nand_wait();
}

void __nand_set_address(s32 page_off, s32 pageno) {
	NAND_debug("nand_set_address: %d, %d\n", page_off, pageno);
	if (page_off != -1) __nand_write32(NAND_ADDR0, page_off);	
	if (pageno != -1) __nand_write32(NAND_ADDR1, pageno);
}

void __nand_setup_dma(u8 *data, u8 *spare) {
	NAND_debug("nand_setup_dma: %p, %p\n", data, spare);
	if (((s32)data) != -1) {
		dc_invalidaterange(data, 0x800);
		__nand_write32(NAND_DATA, (s32)data);
	}
	if (((s32)spare) != -1) {
		dc_invalidaterange(spare, 0x50); // +0x10 for calculated syndrome?
		__nand_write32(NAND_ECC, (s32)spare);
	}
}

int nand_reset(void)
{
	NAND_debug("nand_reset()\n");
	nand_send_command(NAND_RESET, 0, 0x8000, 0);
// yay cargo cult
	__nand_write32(NAND_CONF, 0x8000000);
	__nand_write32(NAND_CONF, 0x4b3e0e7f);
	return 0;
}

u32 nand_get_id(void) {
	STACK_ALIGN(u8, idbuf, 0x40, 64);
	
	__nand_wait();

	NAND_debug("nand_get_id(%p) (pre)\n", idbuf);
	memset(idbuf, 0x42, 4);
	dc_flushrange(idbuf, 0x40);
	__nand_set_address(0,0);

	NAND_debug("id = %02hx%02hx%02hx%02hx\n", idbuf[0], idbuf[1], idbuf[2], idbuf[3]);

	__nand_setup_dma(idbuf, (u8 *)-1);
	nand_send_command(NAND_CHIPID, 1, 0x2000, 0x40);
	NAND_debug("id = %02hx%02hx%02hx%02hx (post)\n", idbuf[0], idbuf[1], idbuf[2], idbuf[3]);

	return idbuf[0] << 24 | idbuf[1] << 16 | idbuf[2] << 8 | idbuf[3];
}

u32 nand_get_status(void) {
	STACK_ALIGN(u8, status_buf, 0x40, 64);
	status_buf[0]=0;
	dc_flushrange(status_buf, 0x40);
	__nand_setup_dma(status_buf, (u8 *)-1);
	nand_send_command(NAND_GETSTATUS, 0, 0x2000, 0x40);
	dc_invalidaterange(status_buf, 0x40);
	return status_buf[0];
}

inline void __nand_wait_irq(void) {
	while(irq == 0);
}

void nand_read_page(u32 pageno, void *data, void *ecc) {
  NAND_debug("nand_read_page(%u, %p, %p)\n", pageno, data, ecc);
#if 1
  irq = 0;
  __nand_set_address(0, pageno);
  nand_send_command(NAND_READ_PRE, 0x1f, 0, 0);

  dc_invalidaterange(data, 0x800);
  dc_invalidaterange(ecc, 0x50);

  __nand_setup_dma(data, ecc);
  nand_send_command(NAND_READ_POST, 0, 0x4000b000, 0x840);

  __nand_wait_irq();
#else
	int i;
	unsigned char *c_data = (unsigned char *)data;
	unsigned char *c_ecc = (unsigned char *)ecc;
	for(i=0; i<0x800; i++) c_data[i] = pageno % 256;
	for(i=0; i<0x40; i++) c_ecc = pageno % 256;
#endif
}


// FIXME: do we really need to read the ECC data?
static u8 ecc[64] __attribute__((aligned(32))) MEM2_BSS;
void nand_read_cluster(u32 clusterno, void *data) {
	int i;
	for(i = 0; i < 8; i++)
		nand_read_page((clusterno*8) + i, data + (i * PAGE_SIZE), ecc);
}

void nand_read_decrypted_cluster(u32 clusterno, void *data) {
	nand_read_cluster(clusterno, data);
	aes_reset();
	aes_set_key(otp.nand_key);
	aes_empty_iv();
	aes_decrypt(data, data, 0x400, 0);
}

#ifdef NAND_SUPPORT_WRITE
void nand_write_page(u32 pageno, void *data, void *ecc) {
	NAND_debug("nand_write_page(%u, %p, %p)\n", pageno, data, ecc);
	dc_flushrange(data, 0x800);
	dc_flushrange(ecc, 0x40);	
	__nand_set_address(0, pageno);
	__nand_setup_dma(data, ecc);
	nand_send_command(NAND_WRITE_PRE, 0x1f, 0x40005000, 0x840);
	
	nand_send_command(NAND_WRITE_POST, 0, 0x40008000, 0x0);
}
#endif

#ifdef NAND_SUPPORT_ERASE
void nand_erase_block(u32 pageno) {
  	NAND_debug("nand_erase_block(%d)\n", pageno);
  	__nand_set_address(0, pageno);
  	nand_send_command(NAND_ERASE_PRE, 0x1c, 0, 0);
	nand_send_command(NAND_ERASE_POST, 0, 0x40008000, 0x0);
}
#endif

void nand_initialize(void)
{
	irq_enable(IRQ_NAND);
	irq = 0;
	nand_reset();
}
