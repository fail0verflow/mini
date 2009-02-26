#ifndef __BOOT2_H__
#define __BOOT2_H__

void boot2_ipc(volatile ipc_request *req);
int boot2_run(u32 tid_hi, u32 tid_lo);
void boot2_init();

#endif
