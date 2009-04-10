#ifndef __BOOT2_H__
#define __BOOT2_H__

#include "ipc.h"

u32 boot2_run(u32 tid_hi, u32 tid_lo);
void boot2_init();
u32 boot2_ipc(volatile ipc_request *req);

#endif
