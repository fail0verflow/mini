#include "types.h"
#include "utils.h"
#include "hollywood.h"
#include "start.h"

#define gpio_delay() udelay(100)


void send_bits(int b, int bits)
{
	while(bits--)
	{
		if(b & (1 << bits))
			set32(HW_GPIO1OUT, 0x1000);
		else
			clear32(HW_GPIO1OUT, 0x1000);
		gpio_delay();
		set32(HW_GPIO1OUT, 0x800);
		gpio_delay();
		clear32(HW_GPIO1OUT, 0x800);
		gpio_delay();
	}
}

int recv_bits(int bits)
{
	int res = 0;
	while(bits--)
	{
		res <<= 1;
		set32(HW_GPIO1OUT, 0x800);
		gpio_delay();
		clear32(HW_GPIO1OUT, 0x800);
		gpio_delay();
		res |= !!(read32(HW_GPIO1IN) & 0x2000);
	}
	return res;
}

int seeprom_read(void *dst, int offset, int size)
{
	int i;
	u16 *ptr = (u16 *)dst;
	u16 recv;

	if(size & 1)
		return -1;

	clear32(HW_GPIO1OUT, 0x800);
	clear32(HW_GPIO1OUT, 0x400);	
	gpio_delay();

	for(i = 0; i < size; ++i)
	{
		set32(HW_GPIO1OUT, 0x400);
		send_bits((0x600 | (offset + i)), 11);
		recv = recv_bits(16);
		*ptr++ = recv;
		clear32(HW_GPIO1OUT, 0x400);
		gpio_delay();
	}

	return size;
}
