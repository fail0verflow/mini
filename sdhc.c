/* parts based on:
 *   * "SD Host Controller driver based on the SD Host Controller Standard" copyright (c) 2006 Uwe Stuehler <uwe@openbsd.org>
 *   * Simplified SD Host Controller Standard
 */

#include "hollywood.h"
#include "sdhc.h"
#include "utils.h"
#include "string.h"
#include "start.h"
#include "memory.h"

#define _READONLY 1

#define		SDHC_PRINT_ERROR	1
//#define		SDHC_DEBUG		1
//#define	SDHC_DEBUG_V		1

#if defined(SDHC_DEBUG_V) && !defined(SDHC_DEBUG)
#	define SDHC_DEBUG	1
#endif

#ifdef SDHC_DEBUG
#	include "gecko.h"
#	define	sdhc_debug(reg, f, arg...) do { gecko_printf("sdhc%d: " f "\n", ((reg - SD_REG_BASE) / 0x100), ##arg); } while(0)
#else
#	define	sdhc_debug(reg, f, arg...) 
#endif

#ifdef SDHC_PRINT_ERROR
#	include "gecko.h"
#	define	sdhc_error(reg, f, arg...) do { gecko_printf("sdhc%d: " f "\n", ((reg - SD_REG_BASE) / 0x100), ##arg); } while(0)
#else
#	define	sdhc_error(reg, f, arg...) 
#endif

#define	SDHC_SOFTWARE_RESET_DAT		(1 << 2)
#define	SDHC_SOFTWARE_RESET_CMD		(1 << 1)
#define	SDHC_SOFTWARE_RESET_ALL		(7)

#define	SDHC_TIMEOUT_MAX		(0x0e)

#define	SDHC_BFREQ_KHZ(c)		((((c) >> 8) & 0x3f) * 1000)

#define	SDHC_CAP_VOLTAGE_33		(1 << 24)
#define	SDHC_CAP_VOLTAGE_30		(1 << 25)
#define	SDHC_CAP_VOLTAGE_18		(1 << 26)

#define	SDHC_CAP_SDMA			(1 << 22)

#define	SDHC_PCTRL_VOLTAGE_SHIFT	1
#define	SDHC_PCTRL_VOLTAGE_33		(0x07 << SDHC_PCTRL_VOLTAGE_SHIFT)
#define	SDHC_PCTRL_VOLTAGE_30		(0x06 << SDHC_PCTRL_VOLTAGE_SHIFT)
#define	SDHC_PCTRL_VOLTAGE_18		(0x05 << SDHC_PCTRL_VOLTAGE_SHIFT)
#define	SDHC_PCTRL_ENABLE		1

#define	SDHC_CLOCK_INTERNAL_ENABLE	(1 << 0)
#define	SDHC_CLOCK_INTERNAL_STABLE	(1 << 1)
#define	SDHC_CLOCK_SD_ENABLE		(1 << 2)

#define	SDHC_CARD_INSERTED		(1 << 16)
#define	SDHC_WRITE_PROTECT		(1 << 19)

#define	SDHC_BLOCKS_MAX			65535

#define	SDHC_CMDMODE_MULTIBLOCK		(1 << 5)
#define	SDHC_CMDMODE_READ		(1 << 4)
#define	SDHC_CMDMODE_WRITE		(0 << 4)
#define	SDHC_CMDMODE_ACMD12_ENABLE	(1 << 2)
#define	SDHC_CMDMODE_BLOCKCNT_ENABLE	(1 << 1)
#define	SDHC_CMDMODE_DMA_ENABLE		(1 << 0)

#define	SDHC_CMD_DATA			(1 << 5)
#define	SDHC_CMD_IDXCHECK		(1 << 4)
#define	SDHC_CMD_CRC			(1 << 3)

#define	SDHC_CMDTEST_READ		0x40

#define	SDHC_CMD_MASK			0xff
#define	SDHC_CMD_SHIFT			8

#define	SDHC_CMD_NORESP			0
#define	SDHC_CMD_RESP_136		1
#define	SDHC_CMD_RESP_48		2
#define	SDHC_CMD_RESP_BUSY		3

#define	SDHC_PRESENT_CMD_INHIBIT_CMD	1
#define	SDHC_PRESENT_CMD_INHIBIT_DAT	2
#define	SDHC_PRESENT_CMD_INHIBIT_BOTH	3

#define	SDHC_BFR_READ_ENABLE		(1 << 11)
#define	SDHC_BFR_WRITE_ENABLE		(1 << 10)

#define	SDHC_WAIT_TIMEOUT					10000
#define	SDHC_WAIT_TIMEOUT_MULTIPLY			50
#define	SDHC_WAIT_TIMEOUT_OUTER				50000
#define	SDHC_WAIT_TIMEOUT_OUTER_MULTIPLY	5


#define	SDHC_INTERRUPT_DMA		(1 << 3)
#define	SDHC_INTERRUPT_TRANSF_COMPLETE	(1 << 1)	

#define	SD_RSP_BUSY			0x0100
#define	SD_RSP_136			0x0200
#define	SD_RSP_CRC			0x0400
#define	SD_RSP_IDX			0x0800
#define	SD_RSP_PRESENT			0x1000

#define	SD_R0				0
#define	SD_R1				(SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_IDX)
#define	SD_R1B				(SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_IDX | SD_RSP_BUSY)
#define	SD_R2				(SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_136)
#define	SD_R3				SD_RSP_PRESENT
#define	SD_R4				SD_RSP_PRESENT
#define	SD_R5				(SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_IDX)
#define	SD_R5B				(SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_IDX | SD_RSP_BUSY)
#define	SD_R6				(SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_IDX)
#define	SD_R7				(SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_IDX)

#define	SD_READ				0x10000

#define	SD_CMD_ACMD			0xC0

#define	SD_CMD_RESET_CARD		 0
#define	SD_CMD_ALL_SEND_CID		 2
#define	SD_CMD_SEND_RELATIVE_ADDR	 3
#define	SD_CMD_SELECT_CARD		 7
#define	SD_CMD_SEND_IF_COND		 8
#define	SD_CMD_SEND_STATUS		13
#define	SD_CMD_SET_BLOCKLEN		16
#define	SD_CMD_READ_MULTIPLE_BLOCK	18
#define	SD_CMD_WRITE_MULTIPLE_BLOCK	25
#define	SD_CMD_APP			55
#define	SD_CMD_READ_OCR			58
#define	SD_CMD_APP_SET_BUS_WIDTH	(SD_CMD_ACMD +  6)
#define	SD_CMD_APP_SEND_OP_COND		(SD_CMD_ACMD + 41)

#define	SDHC_HCR_BUSWIDTH_4		2

#define	BLOCK_SIZE			512
#define	BLOCK_SIZE_512K			7		/* SDMA block size */

#define	SDMA_BLOCK_SIZE			(512 * 1024)

#define MMC_VDD_32_33			0x00100000      /* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34			0x00200000      /* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_29_30			0x00020000      /* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31			0x00040000      /* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_165_195			0x00000080

#define	OCR_POWERUP_STATUS		(1 << 31)
#define	OCR_HCS				(1 << 30)
#define	OCR_CCS				OCR_HCS

#define	INTERRUPT_ERROR			(1 << 15)
#define	INTERRUPT_CARD			(1 <<  8)
#define	INTERRUPT_CARD_REMOVAL		(1 <<  7)
#define	INTERRUPT_CARD_INSERTION	(1 <<  6)
#define	INTERRUPT_BUFFER_READ_READY	(1 <<  5)
#define	INTERRUPT_BUFFER_WRITE_READY	(1 <<  4)
#define	INTERRUPT_DMA			(1 <<  3)
#define	INTERRUPT_BLOCK_GAP_EVENT	(1 <<  2)
#define	INTERRUPT_TRANSFER_COMPLETE	(1 <<  1)
#define	INTERRUPT_COMMAND_COMPLETE	(1 <<  0)

#define	INTERRUPT_ALL			0x81ff

#define	EINTERRUPT_ADMA			(1 <<  9)
#define	EINTERRUPT_ACMD12		(1 <<  8)
#define	EINTERRUPT_CURRENT_LIMIT	(1 <<  7)
#define	EINTERRUPT_DATA_END_BIT		(1 <<  6)
#define	EINTERRUPT_DATA_CRC		(1 <<  5)
#define EINTERRUPT_DATA_TIMEOUT		(1 <<  4)
#define	EINTERRUPT_CMD_INDEX		(1 <<  3)
#define	EINTERRUPT_CMD_END_BIT		(1 <<  2)
#define	EINTERRUPT_CMD_CRC		(1 <<  1)
#define	EINTERRUPT_CMD_TIMEOUT		(1 <<  0)

#define	EINTERRUPT_ALL			0x3ff

u8 __sd_read8(u32 addr)
{
	u32 mask;
	u8 shift;

	shift = (addr & 3) * 8;
	mask = (0xFF << shift);

	return (read32(addr & ~3) & mask) >> shift;
}

u16 __sd_read16(u32 addr)
{
	if(addr & 3)
		return (read32(addr & ~3) & 0xffff0000) >> 16;
	else
		return (read32(addr) & 0xffff);
}

inline u32 __sd_read32(u32 addr)
{
	return read32(addr);
}

void __sd_write8(u32 addr, u8 data)
{
	u32 mask;
	u8 shift;

	shift = (addr & 3) * 8;
	mask = (0xFF << shift);

	mask32(addr & ~3, mask, data << shift);
}

void __sd_write16(u32 addr, u16 data)
{
	if(addr & 3)
		mask32(addr & ~3, 0xffff0000, data << 16);
	else
		mask32(addr, 0xffff, ((u32)data));
}

inline void __sd_write32(u32 addr, u32 data)
{
	write32(addr, data);
}


#if 0
static int __sd_wait8(u32 addr, u8 mask)
{
	u8 timeout = SDHC_WAIT_TIMEOUT_MULTIPLY;
	do
	{
		if(__sd_read8(addr) & mask)
			return 0;
		udelay(SDHC_WAIT_TIMEOUT);
	}
	while(timeout--);
	return SDHC_ETIMEDOUT;
}
#endif

static int __sd_wait16(u32 addr, u16 mask)
{
	u8 timeout = SDHC_WAIT_TIMEOUT_MULTIPLY;
	do
	{
		if(__sd_read16(addr) & mask)
			return 0;
		udelay(SDHC_WAIT_TIMEOUT);
	}
	while(timeout--);
	return SDHC_ETIMEDOUT;
}

#if 0
static int __sd_wait32(u32 addr, u32 mask)
{
	u8 timeout = SDHC_WAIT_TIMEOUT_MULTIPLY;
	do
	{
		if(__sd_read32(addr) & mask)
			return 0;
		udelay(SDHC_WAIT_TIMEOUT);
	}
	while(timeout--);
	return SDHC_ETIMEDOUT;
}
#endif

static int __sd_wait8_r(u32 addr, u8 mask)
{
	u8 timeout = SDHC_WAIT_TIMEOUT_MULTIPLY;
	do
	{
		if(!(__sd_read8(addr) & mask))
			return 0;
		udelay(SDHC_WAIT_TIMEOUT);
	}
	while(timeout--);
	return SDHC_ETIMEDOUT;
}

#if 0
static int __sd_wait16_r(u32 addr, u16 mask)
{
	u8 timeout = SDHC_WAIT_TIMEOUT_MULTIPLY;
	do
	{
		if(!(__sd_read16(addr) & mask))
			return 0;
		udelay(SDHC_WAIT_TIMEOUT);
	}
	while(timeout--);
	return SDHC_ETIMEDOUT;
}
#endif

static int __sd_wait32_r(u32 addr, u32 mask)
{
	u8 timeout = SDHC_WAIT_TIMEOUT_MULTIPLY;
	do
	{
		if(!(__sd_read32(addr) & mask))
			return 0;
		udelay(SDHC_WAIT_TIMEOUT);
	}
	while(timeout--);
	return SDHC_ETIMEDOUT;
}

#ifdef SDHC_DEBUG_V
static void __sd_dumpregs(sdhci_t *sdhci)
{
	sdhc_debug(sdhci->reg_base, " register dump:");
	sdhc_debug(sdhci->reg_base, "  sys addr:    0x%08x", __sd_read32(sdhci->reg_base + SDHC_SDMA_ADDR));
	sdhc_debug(sdhci->reg_base, "  version:     0x%08x", __sd_read16(sdhci->reg_base + SDHC_VERSION));
	sdhc_debug(sdhci->reg_base, "  bsize:       0x%08x", __sd_read16(sdhci->reg_base + SDHC_BLOCK_SIZE));
	sdhc_debug(sdhci->reg_base, "  bcount:      0x%08x", __sd_read16(sdhci->reg_base + SDHC_BLOCK_COUNT));
	sdhc_debug(sdhci->reg_base, "  argument:    0x%08x", __sd_read32(sdhci->reg_base + SDHC_CMD_ARG));
	sdhc_debug(sdhci->reg_base, "  trans mode:  0x%08x", __sd_read16(sdhci->reg_base + SDHC_CMD_TRANSFER_MODE));
	sdhc_debug(sdhci->reg_base, "  pres state:  0x%08x", __sd_read32(sdhci->reg_base + SDHC_PRESENT_STATE));
	sdhc_debug(sdhci->reg_base, "  hc:          0x%08x", __sd_read8(sdhci->reg_base + SDHC_HOST_CONTROL));
	sdhc_debug(sdhci->reg_base, "  pwr ctrl:    0x%08x", __sd_read8(sdhci->reg_base + SDHC_POWER_CONTROL));
	sdhc_debug(sdhci->reg_base, "  gap ctrl:    0x%08x", __sd_read8(sdhci->reg_base + SDHC_BLOCK_GAP_CONTROL));
	sdhc_debug(sdhci->reg_base, "  wup ctrl:    0x%08x", __sd_read8(sdhci->reg_base + SDHC_WAKEUP_CONTROL));
	sdhc_debug(sdhci->reg_base, "  clk ctrl:    0x%08x", __sd_read16(sdhci->reg_base + SDHC_CLOCK_CONTROL));
	sdhc_debug(sdhci->reg_base, "  to ctrl:     0x%08x", __sd_read8(sdhci->reg_base + SDHC_TIMEOUT_CONTROL));
	sdhc_debug(sdhci->reg_base, "  int status:  0x%08x", __sd_read16(sdhci->reg_base + SDHC_NORMAL_INTERRUPT_STATUS));
	sdhc_debug(sdhci->reg_base, "  int enable:  0x%08x", __sd_read16(sdhci->reg_base + SDHC_NORMAL_INTERRUPT_ENABLE));
	sdhc_debug(sdhci->reg_base, "  eint status: 0x%08x", __sd_read16(sdhci->reg_base + SDHC_ERROR_INTERRUPT_STATUS));
	sdhc_debug(sdhci->reg_base, "  eint enable: 0x%08x", __sd_read16(sdhci->reg_base + SDHC_ERROR_INTERRUPT_ENABLE));
	sdhc_debug(sdhci->reg_base, "  caps:        0x%08x", __sd_read32(sdhci->reg_base + SDHC_CAPABILITIES));
	sdhc_debug(sdhci->reg_base, "  max caps:    0x%08x", __sd_read32(sdhci->reg_base + SDHC_MAX_CAPABILITIES));
	sdhc_debug(sdhci->reg_base, "  soft reset:  0x%08x", __sd_read8(sdhci->reg_base + SDHC_SOFTWARE_RESET));
	sdhc_debug(sdhci->reg_base, "  command:     0x%08x", __sd_read16(sdhci->reg_base + SDHC_CMD));
}
#else
#define	__sd_dumpregs(s) while(0) { }
#endif

static int __sd_reset(sdhci_t *sdhci, int all)
{
	u32 caps;
	int retval;
	u32 mask;

	sdhci->is_sdhc = 0;
	sdhci->is_selected = 0;
	sdhci->is_mounted = 0;

	__sd_write16(sdhci->reg_base + SDHC_NORMAL_INTERRUPT_ENABLE, 0);
	__sd_write16(sdhci->reg_base + SDHC_ERROR_INTERRUPT_ENABLE, 0);
	__sd_write16(sdhci->reg_base + SDHC_NORMAL_INTERRUPT_SIGNAL_ENABLE, 0);
	__sd_write16(sdhci->reg_base + SDHC_ERROR_INTERRUPT_SIGNAL_ENABLE, 0);

	if(all)
		mask = SDHC_SOFTWARE_RESET_ALL;
	else
		mask = SDHC_SOFTWARE_RESET_CMD | SDHC_SOFTWARE_RESET_DAT;

	sdhc_error(sdhci->reg_base, "resetting card (mask = %x)", mask);
	sdhc_debug(sdhci->reg_base, "software reset register: %02x", __sd_read8(sdhci->reg_base + SDHC_SOFTWARE_RESET));
	__sd_write8(sdhci->reg_base + SDHC_SOFTWARE_RESET, mask);
	sdhc_debug(sdhci->reg_base, "software reset register: %02x", __sd_read8(sdhci->reg_base + SDHC_SOFTWARE_RESET));
	sdhc_debug(sdhci->reg_base, "waiting for reset bit to be unset");

	retval = __sd_wait8_r(sdhci->reg_base + SDHC_SOFTWARE_RESET, mask);
	if(retval < 0)
	{
		sdhc_debug(sdhci->reg_base, "reset failed, bits were never unset");
		sdhc_debug(sdhci->reg_base, "software reset register: %02x", __sd_read8(sdhci->reg_base + SDHC_SOFTWARE_RESET));
		return retval;
	}

	sdhc_debug(sdhci->reg_base, "reset done");
	
	__sd_dumpregs(sdhci);
	__sd_write8(sdhci->reg_base + SDHC_TIMEOUT_CONTROL, SDHC_TIMEOUT_MAX);

	caps = __sd_read32(sdhci->reg_base + SDHC_CAPABILITIES);
	sdhc_debug(sdhci->reg_base, "capabilites: %X", caps);

	return 0;
}

static int __sd_clock_div(u32 base, u32 target)
{
	int d;
	for(d = 1; d <= 256; d *= 2)
	{
		if((base / d) <= target)
			return (d/2);
	}
	return 256 / 2;
}

static int __sd_clock(sdhci_t *sdhci, u8 enable, u32 freq)
{
	u32 caps;
	int d, retval;
	
	__sd_write16(sdhci->reg_base + SDHC_CLOCK_CONTROL, 0);

	if(!enable)
		return 0;

	caps = read32(sdhci->reg_base + SDHC_CAPABILITIES);
	sdhc_debug(sdhci->reg_base, "capabilites: %X", caps);

	if(SDHC_BFREQ_KHZ(caps) != 0)
		d = __sd_clock_div(SDHC_BFREQ_KHZ(caps), freq);
	else
		d = 256 / 2;

	sdhc_debug(sdhci->reg_base, "using a clock divisor of %d to archieve a speed of %d kHz (base = %d)", 2*d, SDHC_BFREQ_KHZ(caps) / (2 * d), SDHC_BFREQ_KHZ(caps));
	sdhc_debug(sdhci->reg_base, " -> clock_control = %X", (d << 8) | SDHC_CLOCK_INTERNAL_ENABLE);

	__sd_write16(sdhci->reg_base + SDHC_CLOCK_CONTROL, (d << 8) | SDHC_CLOCK_INTERNAL_ENABLE);

	__sd_dumpregs(sdhci);
	sdhc_debug(sdhci->reg_base, "waiting for internal clock to become stable");

	retval = __sd_wait16(sdhci->reg_base + SDHC_CLOCK_CONTROL, SDHC_CLOCK_INTERNAL_STABLE);
	if(retval < 0)
	{
		sdhc_debug(sdhci->reg_base, "clock didn't become stable :/");
		__sd_dumpregs(sdhci);
		return retval;
	}
	sdhc_debug(sdhci->reg_base, "clock is stable; enabling sd clock");

	__sd_write16(sdhci->reg_base + SDHC_CLOCK_CONTROL, (d << 8) | SDHC_CLOCK_SD_ENABLE | SDHC_CLOCK_INTERNAL_ENABLE);
	return 0;
}

static int __sd_power(sdhci_t *sdhci, int vdd)
{
	u32 caps;
	u8 pwr;

	caps = __sd_read32(sdhci->reg_base + SDHC_CAPABILITIES);

	if(vdd == -1)
	{
		if(caps & SDHC_CAP_VOLTAGE_33)
			vdd = SDHC_CAP_VOLTAGE_33;
		else if(caps & SDHC_CAP_VOLTAGE_30)
			vdd = SDHC_CAP_VOLTAGE_30;
		else if(caps & SDHC_CAP_VOLTAGE_18)
			vdd = SDHC_CAP_VOLTAGE_18;
		else
		{
			sdhc_error(sdhci->reg_base, "no voltage supported by the host? this should never happen...");
			return SDHC_ESTRANGE;
		}
	}

	if(!(caps & vdd))
	{
		sdhc_debug(sdhci->reg_base, "voltage %x not supported by the hc");
		return SDHC_EINVAL;
	}


	pwr = 0;
	switch(vdd)
	{
		case SDHC_CAP_VOLTAGE_33:
			pwr |= SDHC_PCTRL_VOLTAGE_33;
			break;
		case SDHC_CAP_VOLTAGE_30:
			pwr |= SDHC_PCTRL_VOLTAGE_30;
			break;
		case SDHC_CAP_VOLTAGE_18:
			pwr |= SDHC_PCTRL_VOLTAGE_18;
			break;
		default:
			sdhc_debug(sdhci->reg_base, "invalid vdd: %x", vdd);
			return SDHC_EINVAL;
	}

	sdhc_debug(sdhci->reg_base, "writing %02x to power control", pwr);
	__sd_write8(sdhci->reg_base + SDHC_POWER_CONTROL, pwr);
	pwr |= SDHC_PCTRL_ENABLE;
	sdhc_debug(sdhci->reg_base, "writing %02x to power control", pwr);
	__sd_write8(sdhci->reg_base + SDHC_POWER_CONTROL, pwr);

	__sd_dumpregs(sdhci);
	sdhc_debug(sdhci->reg_base, "card should get voltage now"); 

	if(!(__sd_read8(sdhci->reg_base + SDHC_POWER_CONTROL) & SDHC_PCTRL_ENABLE))
	{
		sdhc_error(sdhci->reg_base, "pctrl = 0 again");
		return SDHC_ESTRANGE;
	}

	return 0;

}

static s32 __sd_cmd(sdhci_t *sdhci, u32 cmd, u32 type, u32 arg, u32 blk_cnt, void *buffer, u32 *response, u8 rlen)
{
	int mode;
	int command;
	int mask;
	int use_dma;
	int retval;
	int i, imax;
	u32 caps;
#if 0			/* only needed for PIO which is currently broken */
	u8 *ptr;
	int len;
#endif

	if(cmd & SD_CMD_ACMD)
	{
		sdhc_debug(sdhci->reg_base, " cmd %X is ACMD%d, sending CMD55 first", cmd, cmd - SD_CMD_ACMD);
		retval = __sd_cmd(sdhci, SD_CMD_APP, SD_R1, sdhci->rca << 16, 0, NULL, NULL, 0);
		sdhc_debug(sdhci->reg_base, " CMD55: retval = %d", retval);
		// TODO: also check the response here?
		if(retval < 0)
			return retval;
		cmd -= SD_CMD_ACMD;
	}

	__sd_write16(sdhci->reg_base + SDHC_ERROR_INTERRUPT_STATUS, 0);
	__sd_write16(sdhci->reg_base + SDHC_NORMAL_INTERRUPT_STATUS, 0);

	sdhc_debug(sdhci->reg_base, "__sd_cmd: cmd = %X, type = %X, arg = %X, blk_cnt = %d, rlen = %d", cmd, type, arg, blk_cnt, rlen);

	if(blk_cnt > SDHC_BLOCKS_MAX)
	{
		sdhc_debug(sdhci->reg_base, "%d blocks are too much...", blk_cnt);
		return SDHC_EOVERFLOW;
	}

	caps = __sd_read32(sdhci->reg_base + SDHC_CAPABILITIES);
	if(caps & SDHC_CAP_SDMA && ((u32)buffer % 32 == 0) && blk_cnt > 0 && buffer != NULL)
		use_dma = 1;
	else
		use_dma = 0;

	sdhc_debug(sdhci->reg_base, "enable DMA: %d (buffer mod 32: %d)", use_dma, (u32)buffer % 32);

	mode = mask = 0;
	command = (cmd & SDHC_CMD_MASK) << SDHC_CMD_SHIFT;
	if(blk_cnt > 0)
	{
		sdhc_debug(sdhci->reg_base, "block count is > 0; setting up mode register");
		if(use_dma == 1)
			mode |= SDHC_CMDMODE_DMA_ENABLE;
		mode |= SDHC_CMDMODE_MULTIBLOCK;
		mode |= SDHC_CMDMODE_ACMD12_ENABLE;
		mode |= SDHC_CMDMODE_BLOCKCNT_ENABLE;
		if(type & SD_READ)
		{
			sdhc_debug(sdhci->reg_base, "read operation");
			mask = SDHC_BFR_READ_ENABLE;
			mode |= SDHC_CMDMODE_READ;
		}
		else
		{
			sdhc_debug(sdhci->reg_base, "write operation");
			mask = SDHC_BFR_WRITE_ENABLE;
			mode |= SDHC_CMDMODE_WRITE;
		}

		command |= SDHC_CMD_DATA;
	}

	if(!(type & SD_RSP_PRESENT))
		command |= SDHC_CMD_NORESP;
	else if(type & SD_RSP_136)
		command |= SDHC_CMD_RESP_136;
	else if(type & SD_RSP_BUSY)
		command |= SDHC_CMD_RESP_BUSY;
	else
		command |= SDHC_CMD_RESP_48;
	
	if(type & SD_RSP_CRC)
		command |= SDHC_CMD_CRC;
	
	if(type & SD_RSP_IDX)
		command |= SDHC_CMD_IDXCHECK;
	

	sdhc_debug(sdhci->reg_base, "command = %X, mode = %X", command, mode);
	sdhc_debug(sdhci->reg_base, "waiting for command inhibit bits to be cleared..");

	retval = __sd_wait32_r(sdhci->reg_base + SDHC_PRESENT_STATE, SDHC_PRESENT_CMD_INHIBIT_BOTH);
	if(retval < 0)
	{
		sdhc_error(sdhci->reg_base, "command inhibit bits were never cleared");
		__sd_reset(sdhci, 0);
		return retval;
	}

	sdhc_debug(sdhci->reg_base, "command inhibit bits cleared, sending command");

	if(use_dma == 1)
	{
		sdhc_debug(sdhci->reg_base, "preparing buffer for SDMA transfer");
		if(mask == SDHC_BFR_WRITE_ENABLE)
			dc_flushrange(buffer, blk_cnt * BLOCK_SIZE);
		else
			dc_invalidaterange(buffer, blk_cnt * BLOCK_SIZE);
			
		__sd_write32(sdhci->reg_base + SDHC_SDMA_ADDR, (u32)buffer);
		__sd_write16(sdhci->reg_base + SDHC_NORMAL_INTERRUPT_STATUS, 0);
		__sd_write16(sdhci->reg_base + SDHC_ERROR_INTERRUPT_STATUS, 0);
		__sd_write16(sdhci->reg_base + SDHC_NORMAL_INTERRUPT_ENABLE, INTERRUPT_ALL);
		__sd_write16(sdhci->reg_base + SDHC_ERROR_INTERRUPT_ENABLE, EINTERRUPT_ALL);
		__sd_write32(sdhci->reg_base + SDHC_SDMA_ADDR, (u32 )buffer);
	}

	__sd_dumpregs(sdhci);

	if(blk_cnt > 0)
	{
		sdhc_debug(sdhci->reg_base, "writing bsize = %x, bcount = %x", BLOCK_SIZE, blk_cnt);
		__sd_write16(sdhci->reg_base + SDHC_BLOCK_SIZE, (BLOCK_SIZE_512K << 12) | BLOCK_SIZE);
		__sd_write16(sdhci->reg_base + SDHC_BLOCK_COUNT, blk_cnt);
	}

	sdhc_debug(sdhci->reg_base, "writing mode = %x, arg = %x, cmd = %x", mode, arg, command);
	__sd_write32(sdhci->reg_base + SDHC_CMD_ARG, arg);

	// just don't ask....
	__sd_write32(sdhci->reg_base + SDHC_CMD_TRANSFER_MODE, ((u32)command << 16) | mode);

	sdhc_debug(sdhci->reg_base, "writing %08x to %x", ((u32)command << 16) | mode, SDHC_CMD_TRANSFER_MODE);
	sdhc_debug(sdhci->reg_base, "mode = %x", __sd_read16(sdhci->reg_base + SDHC_CMD_TRANSFER_MODE));

	__sd_dumpregs(sdhci);
	sdhc_debug(sdhci->reg_base, "waiting until command phase is done");
	retval = __sd_wait32_r(sdhci->reg_base + SDHC_PRESENT_STATE, SDHC_PRESENT_CMD_INHIBIT_CMD);
	if(retval < 0)
	{
		sdhc_error(sdhci->reg_base, "error: command phase not completed");
		__sd_dumpregs(sdhci);
		__sd_reset(sdhci, 0);
		return retval;
	}
	sdhc_debug(sdhci->reg_base, "command phase is done");

	__sd_dumpregs(sdhci);

	for(i = 0; i < 4; i++)
		sdhc_debug(sdhci->reg_base, "response %d: %X", i, __sd_read32(sdhci->reg_base + SDHC_RESPONSE + 4*i));
	if(rlen < 4 && type & SD_RSP_PRESENT)
	{
		sdhc_debug(sdhci->reg_base, "response buffer not big enough for response...");
	}
	else if(type & SD_RSP_PRESENT)
	{
		if(type & SD_RSP_136)
		{
			u8 *p = (u8 *)response;

			imax = 15 > rlen ? rlen : 15;

			for(i = 0; i < imax; i++)
				*p++ = __sd_read8(sdhci->reg_base + SDHC_RESPONSE + i);

			for(i = 0; i < 4; i++)
				sdhc_debug(sdhci->reg_base, "response %d: %X", i, __sd_read32(sdhci->reg_base + SDHC_RESPONSE + 4*i));
		}
		else
		{
			response[0] = __sd_read32(sdhci->reg_base + SDHC_RESPONSE);
			sdhc_debug(sdhci->reg_base, "response = %08X", response[0]);
		}
		sdhc_debug(sdhci->reg_base, "copied response to buffer");
	}

	// FIXME: check response and abort on errors?
	
	if(blk_cnt > 0)
	{
		sdhc_debug(sdhci->reg_base, "starting transfer of %d %d byte blocks", blk_cnt, BLOCK_SIZE);
		if(use_dma == 0)
		{
			gecko_printf("sdhci: PIO mode is broken, align your buffer and use DMA.\n");
			__sd_reset(sdhci, 0);
			return SDHC_EIO;
#if 0

			sdhc_debug(sdhci->reg_base, "using data port buffer to read/write data");
			len = blk_cnt * BLOCK_SIZE; // as long as BLOCK_SIZE % 4 == 0 len % 4 is (obviously) also 0; this will crash and burn otherwise!!
			ptr = (u8 *)buffer;

			while(len > 0)
			{
				if(len % BLOCK_SIZE == 0)
				{
					sdhc_debug(sdhci->reg_base, "starting to work on the next block; bytes left = %d", len);
					sdhc_debug(sdhci->reg_base, "waiting for data read/write enable bits to be set");
					retval = __sd_wait32(sdhci->reg_base + SDHC_PRESENT_STATE, mask);
					if(retval < 0)
					{
						// FIXME: error handling!!
						sdhc_debug(sdhci->reg_base, "timed out");
						sdhc_debug(sdhci->reg_base, "present state = %08x", __sd_read32(sdhci->reg_base + SDHC_PRESENT_STATE));
						__sd_dumpregs(sdhci);
						return retval;
					}
					sdhc_debug(sdhci->reg_base, "data read/write enable bits are set");
				}
				if(mask == SDHC_BFR_READ_ENABLE)
					*(u32 *)ptr = __sd_read32(sdhci->reg_base + SDHC_DATA);
				else
					__sd_write32(sdhci->reg_base + SDHC_DATA, *(u32 *)ptr);
				ptr += 4;
				len -= 4;
			}
			sdhc_debug(sdhci->reg_base, "all data transferred");

			sdhc_debug(sdhci->reg_base, "waiting.."); // FIXME: interrupts should be used here, don't even know if this works
			//if(mask == SDHC_BFR_READ_ENABLE)
			//	while(__sd_read32(sdhci->reg_base + SDHC_PRESENT_STATE) & (1 << 9));
			//else
			//	while(__sd_read32(sdhci->reg_base + SDHC_PRESENT_STATE) & (1 << 2));
			sdhc_debug(sdhci->reg_base, "done");
#endif
		}
		else	/* SDMA; transfer data in 512 KB blocks (i.e. 1024 512 byte blocks */
		{
			u8 *ptr = (u8 *)buffer;
			sdhc_debug(sdhci->reg_base, "using SDMA to transfer data");

			// poor man's interrupts
			while(1)
			{
				sdhc_debug(sdhci->reg_base, "waiting for interrupts...");
				retval = __sd_wait16(sdhci->reg_base + SDHC_NORMAL_INTERRUPT_STATUS, INTERRUPT_DMA | INTERRUPT_TRANSFER_COMPLETE);
				if(retval < 0)
				{
					sdhc_error(sdhci->reg_base, "failed while waiting for transfer complete or DMA interrupts...");
					__sd_reset(sdhci, 0);
					return retval;
				}

				retval = __sd_read16(sdhci->reg_base + SDHC_NORMAL_INTERRUPT_STATUS);

				if(retval & INTERRUPT_TRANSFER_COMPLETE)
				{
					__sd_dumpregs(sdhci);
					sdhc_debug(sdhci->reg_base, "transfer completed. disabling interrupts again and returning.");
					__sd_write16(sdhci->reg_base + SDHC_NORMAL_INTERRUPT_STATUS, 0);
					__sd_write16(sdhci->reg_base + SDHC_NORMAL_INTERRUPT_ENABLE, 0);
					return 0;
				}
				else if(retval & INTERRUPT_DMA)
				{
					blk_cnt = blk_cnt > (SDMA_BLOCK_SIZE / SDHC_BLOCK_SIZE) ? blk_cnt - (SDMA_BLOCK_SIZE / SDHC_BLOCK_SIZE) : 0;
					__sd_dumpregs(sdhci);
					sdhc_debug(sdhci->reg_base, "DMA interrupt set, updating next SDMA address");
					sdhc_debug(sdhci->reg_base, "sd blocks left: %d, addr: %08x -> %08x", blk_cnt, ptr, ptr + SDMA_BLOCK_SIZE);

					if(blk_cnt == 0)
					{
						sdhc_error(sdhci->reg_base, "FATAL ERROR: hc wants to transfer more DMA data but no more blocks are left.");
						__sd_reset(sdhci, 0);
						return SDHC_EIO;
					}

					__sd_write16(sdhci->reg_base + SDHC_NORMAL_INTERRUPT_STATUS, 0);
					ptr += SDMA_BLOCK_SIZE;
					__sd_write32(sdhci->reg_base + SDHC_SDMA_ADDR, (u32 )ptr);

					sdhc_debug(sdhci->reg_base, "next DMA transfer started.");
					__sd_dumpregs(sdhci);
				}
			}
		}
	}

	return 0;
}

void __sd_print_status(sdhci_t *sdhci)
{
	return;
#ifdef SDHC_DEBUG
	u32 status;
	u32 state;
	int retval;

#if 0
	// FIXME: doesn't work for some reason ?!?
	// sdhc0: Card status: 00000900 [|ò¦|ó¦L]
	char *state_name[] = {
		"IDLE",
		"READY",
		"IDENT",
		"STANDBY",
		"TRANSFER",
		"DATA (SEND)",
		"RECEIVE",
		"PROGRAM",
		"DISCONNECT"
	};

	sdhc_debug(sdhci->reg_base, "Card status %08x [%s]", status, state_name[state]);
#endif


	retval = __sd_cmd(sdhci, SD_CMD_SEND_STATUS, SD_R1, (u32)sdhci->rca << 16, 0, NULL, &status, sizeof(status));
	state = (status >> 9) & 0x0f;
	gecko_printf("sdhc%d: Card status: %08x [", ((sdhci->reg_base - SD_REG_BASE) / 0x100), status);

	switch(state)
	{
		case 0:
			gecko_printf("IDLE]\n");
			break;
		case 1:
			gecko_printf("READY]\n");
			break;
		case 2:
			gecko_printf("IDENT]\n");
			break;
		case 3:
			gecko_printf("STANDBY]\n");
			break;
		case 4:
			gecko_printf("TRANSFER]\n");
			break;
		case 5:
			gecko_printf("DATA (SEND)]\n");
			break;
		case 6:
			gecko_printf("DATA (RECEIVE)]\n");
			break;
		case 7:
			gecko_printf("PROGRAM]\n");
			break;
		case 8:
			gecko_printf("DISCONNECT]\n");
			break;
		default:
			sdhc_debug(sdhci->reg_base, "Reserved]\n", status);
	}

#endif
}

int sd_mount(sdhci_t *sdhci)
{
	u32 caps;
	s32 retval;
	u32 resp[5];
	int tries;

	__sd_dumpregs(sdhci);
	retval = __sd_reset(sdhci, 1);
	if(retval < 0)
		return retval;

	if(!sd_inserted(sdhci))
		return SDHC_ENOCARD;

	caps = __sd_read32(sdhci->reg_base + SDHC_CAPABILITIES);

	retval = __sd_power(sdhci, -1);
	if(retval < 0)
		return retval;

	retval = __sd_clock(sdhci, 1, 25000);
	if(retval < 0)
		return retval;

	sdhc_debug(sdhci->reg_base, "resetting card");
	retval = __sd_cmd(sdhci, SD_CMD_RESET_CARD, SD_R0, 0x0, 0, NULL, NULL, 0);
	if(retval < 0)
		return retval;
	
	// check for SDHC
	retval = __sd_cmd(sdhci, SD_CMD_SEND_IF_COND, SD_R7, 0x1AA, 0, NULL, resp, 6);
	if(retval < 0 || (resp[0] & 0xff) != 0xAA)
	{
		// SDv1 low-capacity card#
		sdhc_error(sdhci->reg_base, "SDv1 low-capacity card deteced. resetting controller and card again.");

		__sd_reset(sdhci, 1);
		retval = __sd_power(sdhci, -1);
		if(retval < 0)
			return retval;

		retval = __sd_clock(sdhci, 1, 25000);
		if(retval < 0)
			return retval;

		retval = __sd_cmd(sdhci, SD_CMD_RESET_CARD, SD_R0, 0x0, 0, NULL, NULL, 0);
		if(retval < 0)
			return retval;

		sdhci->is_sdhc = 0;
		sdhci->ocr = 0;
	}
	else
	{
		// SDHC card deteced
		sdhc_debug(sdhci->reg_base, "SDv2 card detected.");
		sdhci->is_sdhc = 1;
		sdhci->ocr = OCR_HCS;
	}

	
	if(caps & SDHC_CAP_VOLTAGE_33)
		sdhci->ocr |= MMC_VDD_32_33|MMC_VDD_33_34;
	if(caps & SDHC_CAP_VOLTAGE_30)
		sdhci->ocr |= MMC_VDD_29_30|MMC_VDD_30_31;
	if(caps & SDHC_CAP_VOLTAGE_18)
		sdhci->ocr |= MMC_VDD_165_195;

	sdhc_debug(sdhci->reg_base, "waiting for card to finalize power up");
	tries = 0;
	resp[0] = 0;
	while(tries++ <= SDHC_WAIT_TIMEOUT_OUTER_MULTIPLY)
	{
		sdhc_debug(sdhci->reg_base, "attemp %d", tries);
		retval = __sd_cmd(sdhci, SD_CMD_APP_SEND_OP_COND, SD_R3, sdhci->ocr, 0, NULL, resp, 6);
		sdhc_debug(sdhci->reg_base, " retval = %d, response = %08X", retval, resp[0]);
		if(resp[0] & OCR_POWERUP_STATUS)
		{
			sdhc_debug(sdhci->reg_base, "card power up is done.");
			break;
		}
		udelay(SDHC_WAIT_TIMEOUT_OUTER);
	}

	if(!(resp[0] & OCR_POWERUP_STATUS))
	{
		sdhc_error(sdhci->reg_base, "powerup failed, resetting controller.");
		__sd_reset(sdhci, 1);
		return SDHC_EIO;
	}

	sdhci->ocr = resp[0];

	if(sdhci->ocr & OCR_CCS)
	{
		sdhc_debug(sdhci->reg_base, "SDHC card detected, using block instead of byte offset address mode");
		sdhci->is_sdhc = 1;
	}
	else
	{
		sdhc_debug(sdhci->reg_base, "low-capacity SD card detected. using byte offset address mode.");
		sdhci->is_sdhc = 0;
	}

	sdhc_debug(sdhci->reg_base, "sending ALL_SEND_CID command to get connected card");
	retval = __sd_cmd(sdhci, SD_CMD_ALL_SEND_CID, SD_R3, 0, 0, NULL, resp, 6);
	if(retval < 0)
	{
		sdhc_error(sdhci->reg_base, "__sd_cmd returned %d, resetting controller.", retval);
		__sd_reset(sdhci, 1);
		return SDHC_EIO;
	}

	sdhci->cid = resp[0];

	sdhc_debug(sdhci->reg_base, "CID: %08X, requesting RCA", sdhci->cid);
	retval = __sd_cmd(sdhci, SD_CMD_SEND_RELATIVE_ADDR, SD_R6, 0, 0, NULL, resp, 6);
	if(retval < 0)
	{
		sdhc_error(sdhci->reg_base, "failed at getting RCA (%d), resetting controller.", retval);
		__sd_reset(sdhci, 1);
		return SDHC_EIO;
	}

	sdhci->rca = (resp[0] >> 16) & 0xffff;
	sdhc_debug(sdhci->reg_base, "RCA: %04X", sdhci->rca);

	__sd_print_status(sdhci);

	sd_select(sdhci);

	__sd_print_status(sdhci);

	retval = __sd_cmd(sdhci, SD_CMD_SET_BLOCKLEN, SD_R1, BLOCK_SIZE, 0, NULL, NULL, 0);
	if(retval < 0)
	{
		sdhc_debug(sdhci->reg_base, "failed to set the block length to 512bytes (%d)", retval);
		return retval;
	}

	__sd_print_status(sdhci);

	sd_select(sdhci);

	sdhc_debug(sdhci->reg_base, "setting bus width to 4");
	__sd_write8(sdhci->reg_base + SDHC_HOST_CONTROL, __sd_read8(sdhci->reg_base + SDHC_HOST_CONTROL) | SDHC_HCR_BUSWIDTH_4);
	retval = __sd_cmd(sdhci, SD_CMD_APP_SET_BUS_WIDTH, SD_R1, 2, 0, NULL, NULL, 0);
	if(retval < 0)
	{
		sdhc_debug(sdhci->reg_base, "failed to set bus width to 4: %d", retval);
		return retval;
	}

	sdhci->is_mounted = 1;

	return 0;
}

int sd_inserted(sdhci_t *sdhci)
{
	return (__sd_read32(sdhci->reg_base + SDHC_PRESENT_STATE) & SDHC_CARD_INSERTED) == SDHC_CARD_INSERTED;
}

#ifndef _READONLY

int sd_protected(sdhci_t *sdhci)
{
	return (__sd_read32(sdhci->reg_base + SDHC_PRESENT_STATE) & SDHC_WRITE_PROTECT) == SDHC_WRITE_PROTECT;
}

#endif

int sd_init(sdhci_t *sdhci, int slot)
{
	memset(sdhci, 0, sizeof *sdhci);

	if(slot > 1)
		return SDHC_EINVAL;

	sdhci->reg_base = SD_REG_BASE + slot * 0x100;
	return 0;
}

int sd_cmd(sdhci_t *sdhci, u32 cmd, u32 type, u32 arg, u32 blk_cnt, void *buffer, u32 *response, u8 rlen)
{
	return __sd_cmd(sdhci, cmd, type, arg, blk_cnt, buffer, response, rlen);
}

int sd_select(sdhci_t *sdhci)
{
	int retval;

	if(sdhci->is_selected == 1)
		return 0;

	retval = __sd_cmd(sdhci, SD_CMD_SELECT_CARD, SD_R1B, (u32)sdhci->rca << 16, 0, NULL, NULL, 0);

	if(retval < 0)
	{
		sdhc_debug(sdhci->reg_base, "selecting card failed with %d", retval);
		return retval;
	}


	sdhci->is_selected = 1;
	sdhc_debug(sdhci->reg_base, "card selected");

	return 0;
}

int sd_read(sdhci_t *sdhci, u32 start_block, u32 blk_cnt, void *buffer)
{
	int retval;
	u32 response;

	if(sdhci->is_mounted != 1)
		return SDHC_EINVAL;

	retval = sd_select(sdhci);
	if(retval < 0)
		return retval;

	__sd_print_status(sdhci);
	
	if(sdhci->is_sdhc == 0)
		start_block *= 512;

	retval = __sd_cmd(sdhci, SD_CMD_READ_MULTIPLE_BLOCK, SD_R1 | SD_READ, start_block, blk_cnt, buffer, &response, sizeof(response));

	if(retval < 0)
		sdhc_debug(sdhci->reg_base, "reading blocks failed with %d.", retval);
	__sd_print_status(sdhci);

	return retval;
}

#if _READONLY == 1

int sd_write(sdhci_t *sdhci, u32 start_block, u32 blk_cnt, const void *buffer)
{
	int retval;
	u32 response;

	if(sdhci->is_mounted != 1)
		return SDHC_EINVAL;

	retval = sd_select(sdhci);
	if(retval < 0)
		return retval;

	__sd_print_status(sdhci);
	
	if(sdhci->is_sdhc == 0)
		start_block *= 512;

	retval = __sd_cmd(sdhci, SD_CMD_WRITE_MULTIPLE_BLOCK, SD_R1, start_block, blk_cnt, (void *)buffer, &response, sizeof(response));

	if(retval < 0)
		sdhc_debug(sdhci->reg_base, "writing blocks failed with %d.", retval);
	__sd_print_status(sdhci);

	return retval;
}

#endif
