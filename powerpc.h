#ifndef	__POWERPC_H__
#define	__POWERPC_H__	1

#include "ipc.h"

void powerpc_upload_stub(u32 entry);
void powerpc_hang();
void powerpc_reset();
void powerpc_ipc(volatile ipc_request *req);

#endif

