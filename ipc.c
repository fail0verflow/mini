#include <stdarg.h>
#include "types.h"
#include "irq.h"
#include "memory.h"
#include "utils.h"
#include "hollywood.h"
#include "gecko.h"
#include "ipcstruct.h"
#include "ipc.h"

#define IPC_SLOW_SIZE 128

volatile ipc_request slow[IPC_SLOW_SIZE];
u16 slow_head;
vu16 slow_tail;

u16 in_head;
u16 out_tail;

static inline void poke_outtail(u16 num)
{
	write16(HW_IPC_ARMMSG, num);
}
static inline void poke_inhead(u16 num)
{
	write16(HW_IPC_ARMMSG+2, num);
}

static inline u16 peek_intail(void)
{
	return read16(HW_IPC_PPCMSG);
}
static inline u16 peek_outhead(void)
{
	return read16(HW_IPC_PPCMSG+2);
}

void ipc_post(u32 code, u32 tag, u32 num_args, ...)
{
	int arg = 0;
	va_list ap;
	u32 cookie = irq_kill();

	if(peek_outhead() == ((out_tail + 1)&(IPC_OUT_SIZE-1))) {
		gecko_printf("IPC: out queue full, PPC slow/dead/DoSed\n");
		while(peek_outhead() == ((out_tail + 1)&(IPC_OUT_SIZE-1)));
	}
	ipc_out[out_tail].code = code;
	ipc_out[out_tail].tag = tag;
	if(num_args) {
		va_start(ap, num_args);
		while(num_args--) {
			ipc_out[out_tail].args[arg++] = va_arg(ap, u32);
		}
		va_end(ap);
	}
	dc_flushrange((void*)&ipc_out[out_tail], 32);
	out_tail = (out_tail+1)&(IPC_OUT_SIZE-1);
	poke_outtail(out_tail);
	write32(HW_IPC_ARMCTRL, IPC_CTRL_INT_RECV | IPC_CTRL_SEND);

	irq_restore(cookie);
}

static int process_slow(volatile ipc_request *req)
{
	gecko_printf("IPC: process slow @ %p\n",req);

	gecko_printf("IPC: req %08x %08x [%08x %08x %08x %08x %08x %08x]\n", req->code, req->tag,
		req->args[0], req->args[1], req->args[2], req->args[3], req->args[4], req->args[5]);

	gecko_printf("IPC: unknown SLOW request %02x-%04x\n", req->device, req->req);
	return 1;
}

static void process_in(void)
{
	volatile ipc_request *req = &ipc_in[in_head];

	gecko_printf("IPC: process in %d @ %p\n",in_head,req);

	dc_invalidaterange((void*)req, 32);

	gecko_printf("IPC: req %08x %08x [%08x %08x %08x %08x %08x %08x]\n", req->code, req->tag,
		req->args[0], req->args[1], req->args[2], req->args[3], req->args[4], req->args[5]);

	if(req->flags & IPC_FAST) {
		switch(req->device) {
			case IPC_DEV_SYS:
				// handle fast SYS requests here
				switch(req->req) {
					case IPC_SYS_WRITE32:
						write32(req->args[0], req->args[1]);
						break;
					case IPC_SYS_WRITE16:
						write16(req->args[0], req->args[1]);
						break;
					case IPC_SYS_WRITE8:
						write8(req->args[0], req->args[1]);
						break;
					case IPC_SYS_READ32:
						ipc_post(req->code, req->tag, 1, read32(req->args[0]));
						break;
					case IPC_SYS_READ16:
						ipc_post(req->code, req->tag, 1, read16(req->args[0]));
						break;
					case IPC_SYS_READ8:
						ipc_post(req->code, req->tag, 1, read8(req->args[0]));
						break;
					case IPC_SYS_SET32:
						set32(req->args[0], req->args[1]);
						break;
					case IPC_SYS_SET16:
						set16(req->args[0], req->args[1]);
						break;
					case IPC_SYS_SET8:
						set8(req->args[0], req->args[1]);
						break;
					case IPC_SYS_CLEAR32:
						clear32(req->args[0], req->args[1]);
						break;
					case IPC_SYS_CLEAR16:
						clear16(req->args[0], req->args[1]);
						break;
					case IPC_SYS_CLEAR8:
						clear8(req->args[0], req->args[1]);
						break;
					case IPC_SYS_MASK32:
						mask32(req->args[0], req->args[1], req->args[2]);
						break;
					case IPC_SYS_MASK16:
						mask16(req->args[0], req->args[1], req->args[2]);
						break;
					case IPC_SYS_MASK8:
						mask8(req->args[0], req->args[1], req->args[2]);
						break;
					default:
						gecko_printf("IPC: unknown FAST SYS request %04x\n", req->req);
						break;
				}
				break;
			default:
				gecko_printf("IPC: unknown FAST request %02x-%04x\n", req->device, req->req);
				break;
		}
	} else {
		if(slow_head == ((slow_tail + 1)&(IPC_SLOW_SIZE-1))) {
			gecko_printf("IPC: Slowqueue overrun\n");
			panic(0x33);
		}
		slow[slow_tail] = *req;
		slow_tail = (slow_tail+1)&(IPC_SLOW_SIZE-1);
	}
}

void ipc_irq(void)
{
	int donebell = 0;
	gecko_printf("IPC: irq\n");
	while(read32(HW_IPC_ARMCTRL) & IPC_CTRL_RECV) {
		write32(HW_IPC_ARMCTRL, IPC_CTRL_INT_RECV | IPC_CTRL_RECV);
		int donereq = 0;
		while(peek_intail() != in_head) {
			process_in();
			in_head = (in_head+1)&(IPC_IN_SIZE-1);
			poke_inhead(in_head);
			donereq++;
		}
		if(!donereq)
			gecko_printf("IPC bell but no reqs\n");
		donebell++;
	}
	if(!donebell)
		gecko_printf("IPC IRQ but no bell!\n");
}

void ipc_initialize(void)
{
	write32(HW_IPC_ARMMSG, 0);
	write32(HW_IPC_PPCMSG, 0);
	write32(HW_IPC_PPCCTRL, IPC_CTRL_SENT|IPC_CTRL_RECV);
	write32(HW_IPC_ARMCTRL, IPC_CTRL_SENT|IPC_CTRL_RECV);
	slow_head = 0;
	slow_tail = 0;
	in_head = 0;
	out_tail = 0;
	irq_enable(IRQ_IPC);
	write32(HW_IPC_ARMCTRL, IPC_CTRL_INT_RECV);
}
void ipc_shutdown(void)
{
	irq_disable(IRQ_IPC);
}

void ipc_process_slow(void)
{
	while(1) {
		while(slow_head != slow_tail) {
			if(!process_slow(&slow[slow_head]))
				return;
			slow_head = (slow_head+1)&(IPC_SLOW_SIZE-1);
		}
		u32 cookie = irq_kill();
		if(slow_head == slow_tail)
			irq_wait();
		irq_restore(cookie);
	}
}

