#include "ipcstruct.h"

	.section	.rodata.ipc,"a",%progbits
	.globl	ipc_header
	.type	ipc_header, %object
	.size	ipc_header, 32
	.align	5
ipc_header:
	.ascii	"IPC1"
	.long	0
	.long	ipc_in
	.long	32
	.long	ipc_out
	.long	32
	
	.section	.bss.ipc,"aw",%nobits
	.globl	ipc_in
	.type	ipc_in, %object
	.size	ipc_in, 32 * IPC_IN_SIZE
	.align	5
ipc_in:
	.space 32 * IPC_IN_SIZE

	.globl	ipc_out
	.type	ipc_out, %object
	.size	ipc_out, 32 * IPC_OUT_SIZE
	.align	5
ipc_out:
	.space 32 * IPC_IN_SIZE
