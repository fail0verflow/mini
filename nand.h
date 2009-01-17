#ifndef __NAND_H__
#define __NAND_H__

#include "types.h"

void nand_irq(void);

void nand_send_command(u32 command, u32 bitmask, u32 flags, u32 num_bytes);
int nand_reset(void);
u32 nand_get_id(void);
u32 nand_get_status(void);
void nand_read_page(u32 pageno, void *data, void *ecc);
void nand_write_page(u32 pageno, void *data, void *ecc);
void nand_erase_block(u32 pageno);

void nand_read_cluster(u32 clusterno, void *data);
void nand_read_decrypted_cluster(u32 clusterno, void *data);

void nand_initialize();

#endif
