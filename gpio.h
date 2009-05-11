/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	GPIO pin-out constants

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __GPIO_H__
#define __GPIO_H__

enum {
	GP_POWER		= 0x000001,
	GP_SHUTDOWN		= 0x000002,
	GP_FAN			= 0x000004,
	GP_DCDC			= 0x000008,
	GP_DISPIN		= 0x000010,
	GP_SLOTLED		= 0x000020,
	GP_EJECTBTN		= 0x000040,
	GP_SLOTIN		= 0x000080,
	GP_SENSORBAR		= 0x000100,
	GP_DOEJECT		= 0x000200,
	GP_EEP_CS		= 0x000400,
	GP_EEP_CLK		= 0x000800,
	GP_EEP_MOSI		= 0x001000,
	GP_EEP_MISO		= 0x002000,
	GP_AVE_SCL		= 0x004000,
	GP_AVE_SDA		= 0x008000,
	GP_DEBUG0		= 0x010000,
	GP_DEBUG1		= 0x020000,
	GP_DEBUG2		= 0x040000,
	GP_DEBUG3		= 0x080000,
	GP_DEBUG4		= 0x100000,
	GP_DEBUG5		= 0x200000,
	GP_DEBUG6		= 0x400000,
	GP_DEBUG7		= 0x800000,
};

#define GP_DEBUG_SHIFT 16
#define GP_DEBUG_MASK 0xFF0000

#define GP_ALL 0xFFFFFF
#define GP_OWNER_PPC (GP_AVE_SDA | GP_AVE_SCL | GP_DOEJECT | GP_SENSORBAR | GP_SLOTIN | GP_SLOTLED)
#define GP_OWNER_ARM (GP_ALL ^ GP_OWNER_PPC)
#define GP_INPUTS (GP_POWER | GP_EJECTBTN | GP_SLOTIN | GP_EEP_MISO | GP_AVE_SDA)
#define GP_OUTPUTS (GP_ALL ^ GP_INPUTS)
#define GP_ARM_INPUTS (GP_INPUTS & GP_OWNER_ARM)
#define GP_PPC_INPUTS (GP_INPUTS & GP_OWNER_PPC)
#define GP_ARM_OUTPUTS (GP_OUTPUTS & GP_OWNER_ARM)
#define GP_PPC_OUTPUTS (GP_OUTPUTS & GP_OWNER_PPC)
#define GP_DEFAULT_ON (GP_AVE_SCL | GP_DCDC | GP_FAN)
#define GP_ARM_DEFAULT_ON (GP_DEFAULT_ON & GP_OWNER_ARM)
#define GP_PPC_DEFAULT_ON (GP_DEFAULT_ON & GP_OWNER_PPC)

#endif

