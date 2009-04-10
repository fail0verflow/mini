#include "types.h"
#include "irq.h"
#include "start.h"
#include "vsprintf.h"
#include "string.h"
#include "utils.h"
#include "hollywood.h"
#include "powerpc.h"
#include "powerpc_elf.h"
#include "gecko.h"

static u8 gecko_console_enabled = 0;

// These two don't really seem to be needed
// Maybe only for boot buffer or some PPC stuff
static inline void _gecko_get(void)
{
	//set32(HW_EXICTRL, 1);
}

static inline void _gecko_release(void)
{
	//clear32(HW_EXICTRL, 1);
}

static u32 _gecko_command(u32 command)
{
	u32 i;
	// Memory Card Port B (Channel 1, Device 0, Frequency 3 (32Mhz Clock))
	write32(EXI1_CSR, 0xd0);
	write32(EXI1_DATA, command);
	write32(EXI1_CR, 0x19);
	i = 1000;
	while ((read32(EXI1_CR) & 1) && (i--));
	i = read32(EXI1_DATA);
	write32(EXI1_CSR, 0);
	return i;
}

static u32 _gecko_sendbyte(u8 sendbyte)
{
	u32 i = 0;
	i = _gecko_command(0xB0000000 | (sendbyte<<20));
	if (i&0x04000000)
		return 1; // Return 1 if byte was sent
	return 0;
}

static u32 _gecko_recvbyte(u8 *recvbyte)
{
	u32 i = 0;
	*recvbyte = 0;
	i = _gecko_command(0xA0000000);
	if (i&0x08000000) {
		// Return 1 if byte was received
		*recvbyte = (i>>16)&0xff;
		return 1;
	}
	return 0;
}

#if 0
static u32 _gecko_checksend(void)
{
	u32 i = 0;
	i = _gecko_command(0xC0000000);
	if (i&0x04000000)
		return 1; // Return 1 if safe to send
	return 0;
}
#endif

static u32 _gecko_checkrecv(void)
{
	u32 i = 0;
	i = _gecko_command(0xD0000000);
	if (i&0x04000000)
		return 1; // Return 1 if safe to recv
	return 0;
}

static int gecko_isalive(void)
{
	u32 i = 0;
	i = _gecko_command(0x90000000);
	if (i&0x04700000)
		return 1;
	return 0;
}

#if 0
static void gecko_flush(void)
{
	char tmp;
	while(_gecko_recvbyte(&tmp));
}

static int gecko_recvbuffer(void *buffer, u32 size)
{
	u32 left = size;
	char *ptr = (char*)buffer;

	_gecko_get();
	while(left>0) {
		if(!_gecko_recvbyte(ptr))
			break;
		ptr++;
		left--;
	}
	_gecko_release();
	return (size - left);
}
#endif

static int gecko_sendbuffer(const void *buffer, u32 size)
{
	u32 left = size;
	char *ptr = (char*)buffer;

	_gecko_get();
	while(left>0) {
		if(!_gecko_sendbyte(*ptr))
			break;
		ptr++;
		left--;
	}
	_gecko_release();
	return (size - left);
}

#if 0
static int gecko_recvbuffer_safe(void *buffer, u32 size)
{
	u32 left = size;
	char *ptr = (char*)buffer;
	
	_gecko_get();
	while(left>0) {
		if(_gecko_checkrecv()) {
			if(!_gecko_recvbyte(ptr))
				break;
			ptr++;
			left--;
		}
	}
	_gecko_release();
	return (size - left);
}

static int gecko_sendbuffer_safe(const void *buffer, u32 size)
{
	u32 left = size;
	char *ptr = (char*)buffer;

	if((read32(HW_EXICTRL) & EXICTRL_ENABLE_EXI) == 0)
		return left;
	
	_gecko_get();
	while(left>0) {
		if(_gecko_checksend()) {
			if(!_gecko_sendbyte(*ptr))
				break;
			ptr++;
			left--;
		}
	}
	_gecko_release();
	return (size - left);
}
#endif

void gecko_init(void)
{
	write32(EXI0_CSR, 0);
	write32(EXI1_CSR, 0);
	write32(EXI2_CSR, 0);
	write32(EXI0_CSR, 0x2000);
	write32(EXI0_CSR, 3<<10);
	write32(EXI1_CSR, 3<<10);

	if (!gecko_isalive())
		return;

	gecko_console_enabled = 1;
}

u8 gecko_enable_console(const u8 enable)
{
	if (enable) {
		if (gecko_isalive())
			gecko_console_enabled = 1;
	} else
		gecko_console_enabled = 0;

	return gecko_console_enabled;
}

int gecko_printf(const char *fmt, ...)
{
	if (!gecko_console_enabled)
		return 0;

	va_list args;
	char buffer[256];
	int i;

	va_start(args, fmt);
	i = vsprintf(buffer, fmt, args);
	va_end(args);

	return gecko_sendbuffer(buffer, i);
}

// irq context

#define GECKO_STATE_NONE 0
#define GECKO_STATE_RECEIVE_ELF_SIZE 1
#define GECKO_STATE_RECEIVE_ELF 2

static u32 _gecko_cmd = 0;
static u32 _gecko_cmd_start_time = 0;
static u32 _gecko_state = GECKO_STATE_NONE;
static u32 _gecko_receive_left = 0;
static u32 _gecko_receive_len = 0;
static u8 *_gecko_receive_buffer = NULL;

void gecko_timer_initialize(void)
{
	if (!gecko_isalive())
		return;

	irq_set_alarm(100, 1);
}

void gecko_timer(void) {
	u8 b;

	if (_gecko_cmd_start_time && read32(HW_TIMER) >
			(_gecko_cmd_start_time + IRQ_ALARM_MS2REG(5000))) {
		// time's over, bitch
		irq_set_alarm(100, 0);
		_gecko_cmd = 0;
		_gecko_cmd_start_time = 0;
		_gecko_state = GECKO_STATE_NONE;

		return;
	}

	switch (_gecko_state) {
	case GECKO_STATE_NONE:
		if (!_gecko_checkrecv() || !_gecko_recvbyte(&b))
			return;

		_gecko_cmd <<= 8;
		_gecko_cmd |= b;

		switch (_gecko_cmd) {
		// upload powerpc ELF
		case 0x43524150:
			_gecko_state = GECKO_STATE_RECEIVE_ELF_SIZE;
			_gecko_receive_len = 0;
			_gecko_receive_left = 4;

			irq_set_alarm(1, 0);
			_gecko_cmd_start_time = read32(HW_TIMER);

			gecko_console_enabled = 0;

			break;
		}

		return;

	case GECKO_STATE_RECEIVE_ELF_SIZE:
		if (!_gecko_checkrecv() || !_gecko_recvbyte(&b))
			return;

		_gecko_receive_len <<= 8;
		_gecko_receive_len |= b;
		_gecko_receive_left--;

		if (!_gecko_receive_left) {
			_gecko_state = GECKO_STATE_RECEIVE_ELF;
			_gecko_receive_buffer = (u8 *) 0x10100000;
			_gecko_receive_left = _gecko_receive_len;

			powerpc_hang();
		}

		return;

	case GECKO_STATE_RECEIVE_ELF:
		while (_gecko_receive_left) {
			if (!_gecko_checkrecv() || !_gecko_recvbyte(_gecko_receive_buffer))
				return;

			_gecko_receive_buffer++;
			_gecko_receive_left--;
		}

		if (!_gecko_receive_left) {
			irq_set_alarm(100, 0);
			_gecko_cmd = 0;
			_gecko_cmd_start_time = 0;
			_gecko_state = GECKO_STATE_NONE;

			gecko_console_enabled = 1;

			if (powerpc_boot_mem((u8 *) 0x10100000, _gecko_receive_len))
				gecko_printf("GECKOTIMER: elflolwtf?\n");
		}

		return;

	default:
		gecko_printf("GECKOTIMER: statelolwtf?\n");
		break;
	}
}

