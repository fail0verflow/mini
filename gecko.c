/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	USBGecko support code

Copyright (c) 2008		Nuke - <wiinuke@gmail.com>
Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>
Copyright (C) 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/
#ifdef CAN_HAZ_USBGECKO

#include "types.h"
#include "irq.h"
#include "start.h"
#include "vsprintf.h"
#include "string.h"
#include "utils.h"
#include "hollywood.h"
#include "elf.h"
#include "powerpc.h"
#include "powerpc_elf.h"
#include "gecko.h"

static u8 gecko_found = 0;
static u8 gecko_console_enabled = 0;

static u32 _gecko_command(u32 command)
{
	u32 i;
	// Memory Card Port B (Channel 1, Device 0, Frequency 3 (32Mhz Clock))
	write32(EXI1_CSR, 0xd0);
	write32(EXI1_DATA, command);
	write32(EXI1_CR, 0x19);
	while (read32(EXI1_CR) & 1);
	i = read32(EXI1_DATA);
	write32(EXI1_CSR, 0);
	return i;
}

static u32 _gecko_getid(void)
{
	u32 i;
	// Memory Card Port B (Channel 1, Device 0, Frequency 3 (32Mhz Clock))
	write32(EXI1_CSR, 0xd0);
	write32(EXI1_DATA, 0);
	write32(EXI1_CR, 0x19);
	while (read32(EXI1_CR) & 1);
	write32(EXI1_CR, 0x39);
	while (read32(EXI1_CR) & 1);
	i = read32(EXI1_DATA);
	write32(EXI1_CSR, 0);
	return i;
}

#ifndef NDEBUG
static u32 _gecko_sendbyte(u8 sendbyte)
{
	u32 i = 0;
	i = _gecko_command(0xB0000000 | (sendbyte<<20));
	if (i&0x04000000)
		return 1; // Return 1 if byte was sent
	return 0;
}
#endif

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

#if !defined(NDEBUG) && defined(GECKO_SAFE)
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
	u32 i;

	i = _gecko_getid();
	if (i != 0x00000000)
		return 0;

	i = _gecko_command(0x90000000);
	if ((i & 0xFFFF0000) != 0x04700000)
		return 0;

	return 1;
}

static void gecko_flush(void)
{
	u8 tmp;
	while(_gecko_recvbyte(&tmp));
}

#if 0
static int gecko_recvbuffer(void *buffer, u32 size)
{
	u32 left = size;
	char *ptr = (char*)buffer;

	while(left>0) {
		if(!_gecko_recvbyte(ptr))
			break;
		ptr++;
		left--;
	}
	return (size - left);
}
#endif

#if !defined(NDEBUG) && !defined(GECKO_SAFE)
static int gecko_sendbuffer(const void *buffer, u32 size)
{
	u32 left = size;
	char *ptr = (char*)buffer;

	while(left>0) {
		if(!_gecko_sendbyte(*ptr))
			break;
		if(*ptr == '\n') {
#ifdef GECKO_LFCR
			_gecko_sendbyte('\r');
#endif
			break;
		}
		ptr++;
		left--;
	}
	return (size - left);
}
#endif

#if 0
static int gecko_recvbuffer_safe(void *buffer, u32 size)
{
	u32 left = size;
	char *ptr = (char*)buffer;
	
	while(left>0) {
		if(_gecko_checkrecv()) {
			if(!_gecko_recvbyte(ptr))
				break;
			ptr++;
			left--;
		}
	}
	return (size - left);
}
#endif

#if !defined(NDEBUG) && defined(GECKO_SAFE)
static int gecko_sendbuffer_safe(const void *buffer, u32 size)
{
	u32 left = size;
	char *ptr = (char*)buffer;

	if((read32(HW_EXICTRL) & EXICTRL_ENABLE_EXI) == 0)
		return left;
	
	while(left>0) {
		if(_gecko_checksend()) {
			if(!_gecko_sendbyte(*ptr))
				break;
			if(*ptr == '\n') {
#ifdef GECKO_LFCR
				_gecko_sendbyte('\r');
#endif
				break;
			}
			ptr++;
			left--;
		}
	}
	return (size - left);
}
#endif

void gecko_init(void)
{
	write32(EXI0_CSR, 0);
	write32(EXI1_CSR, 0);
	write32(EXI2_CSR, 0);

	if (!gecko_isalive())
		return;

	gecko_found = 1;

	gecko_flush();
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

#ifndef NDEBUG
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

#ifdef GECKO_SAFE
	return gecko_sendbuffer_safe(buffer, i);
#else
	return gecko_sendbuffer(buffer, i);
#endif
}
#endif

// irq context

#define GECKO_STATE_NONE 0
#define GECKO_STATE_RECEIVE_BUFFER_SIZE 1
#define GECKO_STATE_RECEIVE_BUFFER 2

#define GECKO_BUFFER_MAX (20 * 1024 * 1024)

#define GECKO_CMD_BIN_ARM 0x4241524d
#define GECKO_CMD_BIN_PPC 0x42505043

static u32 _gecko_cmd = 0;
static u32 _gecko_cmd_start_time = 0;
static u32 _gecko_state = GECKO_STATE_NONE;
static u32 _gecko_receive_left = 0;
static u32 _gecko_receive_len = 0;
static u8 *_gecko_receive_buffer = NULL;

void gecko_process(void) {
	u8 b;

	if (!gecko_found)
		return;

	if (_gecko_cmd_start_time && read32(HW_TIMER) >
			(_gecko_cmd_start_time + IRQ_ALARM_MS2REG(5000)))
		goto cleanup;

	switch (_gecko_state) {
	case GECKO_STATE_NONE:
		if (!_gecko_checkrecv() || !_gecko_recvbyte(&b))
			return;

		_gecko_cmd <<= 8;
		_gecko_cmd |= b;

		switch (_gecko_cmd) {
		case GECKO_CMD_BIN_ARM:
			_gecko_state = GECKO_STATE_RECEIVE_BUFFER_SIZE;
			_gecko_receive_len = 0;
			_gecko_receive_left = 4;
			_gecko_receive_buffer = (u8 *) 0x0; // yarly

			_gecko_cmd_start_time = read32(HW_TIMER);

			break;

		case GECKO_CMD_BIN_PPC:
			_gecko_state = GECKO_STATE_RECEIVE_BUFFER_SIZE;
			_gecko_receive_len = 0;
			_gecko_receive_left = 4;
			_gecko_receive_buffer = (u8 *) 0x10100000;

			_gecko_cmd_start_time = read32(HW_TIMER);

			break;
		}

		return;

	case GECKO_STATE_RECEIVE_BUFFER_SIZE:
		if (!_gecko_checkrecv() || !_gecko_recvbyte(&b))
			return;

		_gecko_receive_len <<= 8;
		_gecko_receive_len |= b;
		_gecko_receive_left--;

		if (!_gecko_receive_left) {
			if (_gecko_receive_len > GECKO_BUFFER_MAX)
				goto cleanup;

			_gecko_state = GECKO_STATE_RECEIVE_BUFFER;
			_gecko_receive_left = _gecko_receive_len;

			// sorry pal, that memory is mine now
			powerpc_hang();
		}

		return;

	case GECKO_STATE_RECEIVE_BUFFER:
		while (_gecko_receive_left) {
			if (!_gecko_checkrecv() || !_gecko_recvbyte(_gecko_receive_buffer))
				return;

			_gecko_receive_buffer++;
			_gecko_receive_left--;
		}

		if (!_gecko_receive_left)
			break;

		return;

	default:
		gecko_printf("MINI/GECKO: internal error\n");
		return;
	}

	ioshdr *h;

	// done receiving, handle the command
	switch (_gecko_cmd) {
	case GECKO_CMD_BIN_ARM:
		h = (ioshdr *) (u32 *) 0x0;

		if (h->hdrsize != sizeof (ioshdr))
			goto cleanup;

		if (memcmp("\x7F" "ELF\x01\x02\x01",
					(void *) (h->hdrsize + h->loadersize), 7))
			goto cleanup;

		ipc_enqueue_slow(IPC_DEV_SYS, IPC_SYS_JUMP, 1, h->hdrsize);
		break;

	case GECKO_CMD_BIN_PPC:
		ipc_enqueue_slow(IPC_DEV_PPC, IPC_PPC_BOOT, 3,
							1, (u32) 0x10100000, _gecko_receive_len);
		break;
	}

cleanup:
	gecko_flush();

	_gecko_cmd = 0;
	_gecko_cmd_start_time = 0;
	_gecko_state = GECKO_STATE_NONE;
}

#endif

