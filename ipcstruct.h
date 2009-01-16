#ifndef __IPCSTRUCT_H__
#define __IPCSTRUCT_H__

#define IPC_IN_SIZE		32
#define IPC_OUT_SIZE	32

#ifndef _LANGUAGE_ASSEMBLY

#include "ipc.h"

extern volatile ipc_request ipc_in[IPC_IN_SIZE];
extern volatile ipc_request ipc_out[IPC_OUT_SIZE];

#endif

#endif
