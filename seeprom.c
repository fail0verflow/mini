/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	SEEPROM support

Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "types.h"
#include "utils.h"
#include "hollywood.h"
#include "start.h"
#include "gpio.h"

#define eeprom_delay() udelay(5)

void send_bits(int b, int bits)
{
	while(bits--)
	{
		if(b & (1 << bits))
			set32(HW_GPIO1OUT, GP_EEP_MOSI);
		else
			clear32(HW_GPIO1OUT, GP_EEP_MOSI);
		eeprom_delay();
		set32(HW_GPIO1OUT, GP_EEP_CLK);
		eeprom_delay();
		clear32(HW_GPIO1OUT, GP_EEP_CLK);
		eeprom_delay();
	}
}

int recv_bits(int bits)
{
	int res = 0;
	while(bits--)
	{
		res <<= 1;
		set32(HW_GPIO1OUT, GP_EEP_CLK);
		eeprom_delay();
		clear32(HW_GPIO1OUT, GP_EEP_CLK);
		eeprom_delay();
		res |= !!(read32(HW_GPIO1IN) & GP_EEP_MISO);
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

	clear32(HW_GPIO1OUT, GP_EEP_CLK);
	clear32(HW_GPIO1OUT, GP_EEP_CS);
	eeprom_delay();

	for(i = 0; i < size; ++i)
	{
		set32(HW_GPIO1OUT, GP_EEP_CS);
		send_bits((0x600 | (offset + i)), 11);
		recv = recv_bits(16);
		*ptr++ = recv;
		clear32(HW_GPIO1OUT, GP_EEP_CS);
		eeprom_delay();
	}

	return size;
}

