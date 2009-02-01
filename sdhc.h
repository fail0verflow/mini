#ifndef __SDHC_H__
#define __SDHC_H__

#include "types.h"
#include "ipc.h"

#define	SDHC_ENOCARD	-0x1001
#define	SDHC_ESTRANGE	-0x1002
#define	SDHC_EOVERFLOW	-0x1003
#define	SDHC_ETIMEDOUT	-0x1004
#define	SDHC_EINVAL	-0x1005
#define	SDHC_EIO	-0x1006

typedef struct
{
	u32 reg_base;

	u8 is_sdhc;
	u8 is_selected;
	u8 is_mounted;

	u16 rca;
	u32 ocr;
	u32 cid;
} sdhci_t;

int sd_init(sdhci_t *sdhci, int slot);
int sd_reset(sdhci_t *sdhci);
int sd_inserted(sdhci_t *sdhci);
int sd_protected(sdhci_t *sdhci);
int sd_mount(sdhci_t *sdhci);
int sd_cmd(sdhci_t *sdhci, u32 cmd, u32 rsp_type, u32 arg, u32 blk_cnt, void *buffer, u32 *response, u8 rlen);
int sd_select(sdhci_t *sdhci);
int sd_read(sdhci_t *sdhci, u32 start_block, u32 blk_cnt, void *buffer);
int sd_write(sdhci_t *sdhci, u32 start_block, u32 blk_cnt, const void *buffer);

void sd_irq(void);
void sd_initialize();

void sd_ipc(volatile ipc_request *req);

u8 __sd_read8(u32 addr);
u16 __sd_read16(u32 addr);
u32 __sd_read32(u32 addr);

#endif
