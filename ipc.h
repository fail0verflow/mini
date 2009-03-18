#ifndef __IPC_H__
#define __IPC_H__

#include "types.h"

#define IPC_FAST	0x01
#define IPC_SLOW	0x00

#define IPC_DEV_SYS	0x00
#define IPC_DEV_NAND	0x01
#define IPC_DEV_SD	0x02
#define IPC_DEV_KEYS	0x03
#define IPC_DEV_AES	0x04
#define IPC_DEV_BOOT2	0x05

#define IPC_SYS_PING	0x0000
#define IPC_SYS_JUMP	0x0001
#define IPC_SYS_WRITE32	0x0100
#define IPC_SYS_WRITE16	0x0101
#define IPC_SYS_WRITE8	0x0102
#define IPC_SYS_READ32	0x0103
#define IPC_SYS_READ16	0x0104
#define IPC_SYS_READ8	0x0105
#define IPC_SYS_SET32	0x0106
#define IPC_SYS_SET16	0x0107
#define IPC_SYS_SET8	0x0108
#define IPC_SYS_CLEAR32	0x0109
#define IPC_SYS_CLEAR16	0x010a
#define IPC_SYS_CLEAR8	0x010b
#define IPC_SYS_MASK32	0x010c
#define IPC_SYS_MASK16	0x010d
#define IPC_SYS_MASK8	0x010e

#define IPC_NAND_RESET	0x0000
#define IPC_NAND_GETID	0x0001
#define IPC_NAND_READ	0x0002
#define IPC_NAND_WRITE	0x0003
#define IPC_NAND_ERASE	0x0004
#define IPC_NAND_STATUS	0x0005

#define IPC_SD_MOUNT	0x0000
#define IPC_SD_SELECT	0x0001
#define IPC_SD_GETSTATE	0x0002
#define IPC_SD_READ		0x0003
#define IPC_SD_WRITE	0x0004

#define IPC_KEYS_GETOTP	0x0000
#define IPC_KEYS_GETEEP	0x0001

#define IPC_AES_RESET	0x0000
#define IPC_AES_SETIV	0x0001
#define	IPC_AES_SETKEY	0x0002
#define	IPC_AES_DECRYPT	0x0003

#define IPC_BOOT2_RUN	0x0000

#define IPC_CODE (f,d,r) (((f)<<24)|((d)<<16)|(r))

#define IPC_IN_SIZE		32
#define IPC_OUT_SIZE	32
#define IPC_SLOW_SIZE	128

typedef struct {
	union {
		struct {
			u8 flags;
			u8 device;
			u16 req;
		};
		u32 code;
	};
	u32 tag;
	u32 args[6];
} ipc_request;

typedef const struct {
	char magic[3];
	char version;
	void *mem2_boundary;
	volatile ipc_request *ipc_in;
	u32 ipc_in_size;
	volatile ipc_request *ipc_out;
	u32 ipc_out_size;
} ipc_infohdr;

void ipc_irq(void);

void ipc_initialize(void);
void ipc_shutdown(void);
void ipc_post(u32 code, u32 tag, u32 num_args, ...);
void ipc_flush(void);
void ipc_process_slow(void);

#endif
