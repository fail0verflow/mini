#include "types.h"
#include "start.h"
#include "vsprintf.h"
#include "string.h"
#include "utils.h"
#include "hollywood.h"

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

static u32 _gecko_sendbyte(char sendbyte)
{
	u32 i = 0;
	i = _gecko_command(0xB0000000 | (sendbyte<<20));
	if (i&0x04000000)
		return 1; // Return 1 if byte was sent
	return 0;
}

static u32 _gecko_recvbyte(char *recvbyte)
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

static u32 _gecko_checksend(void)
{
	u32 i = 0;
	i = _gecko_command(0xC0000000);
	if (i&0x04000000)
		return 1; // Return 1 if safe to send
	return 0;
}

static u32 _gecko_checkrecv(void)
{
	u32 i = 0;
	i = _gecko_command(0xD0000000);
	if (i&0x04000000)
		return 1; // Return 1 if safe to recv
	return 0;
}

void gecko_init(void)
{
	write32(EXI0_CSR, 0);
	write32(EXI1_CSR, 0);
	write32(EXI2_CSR, 0);
	write32(EXI0_CSR, 0x2000);
	write32(EXI0_CSR, 3<<10);
	write32(EXI1_CSR, 3<<10);
}

void gecko_flush(void)
{
	char tmp;
	while(_gecko_recvbyte(&tmp));
}

int gecko_isalive(void)
{
	u32 i = 0;
	i = _gecko_command(0x90000000);
	if (i&0x04700000)
		return 1;
	return 0;
}

int gecko_recvbuffer(void *buffer, u32 size)
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

int gecko_sendbuffer(const void *buffer, u32 size)
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

int gecko_recvbuffer_safe(void *buffer, u32 size)
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

int gecko_sendbuffer_safe(const void *buffer, u32 size)
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

int gecko_putchar(int ic)
{
	char b = ic;
	return gecko_sendbuffer(&b, 1);
}

int gecko_getchar(void)
{
	char b;
	if(gecko_recvbuffer_safe(&b, 1) != 1)
		return -1;
	return b;
}

int gecko_puts(const char *s)
{
	//udelay(10000);
	return gecko_sendbuffer_safe(s, strlen(s));
}

int gecko_printf( const char *fmt, ...)
{
	va_list args;
	char buffer[1024];
	int i;

	va_start(args, fmt);
	i = vsprintf(buffer, fmt, args);
	va_end(args);
	gecko_puts(buffer);
	return i;
}
