/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	linker script
	
Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

OUTPUT_FORMAT("elf32-bigarm")
OUTPUT_ARCH(arm)
EXTERN(_start)
EXTERN(__ipc_info)
ENTRY(_start)

__stack_size = 0x800;
__irqstack_size = 0x100;
__excstack_size = 0x100;

MEMORY {
	sram		: ORIGIN = 0xffff0000, LENGTH = 64K
	sram2		: ORIGIN = 0xfffe0000, LENGTH = 16K
	pagetable	: ORIGIN = 0xfffe4000, LENGTH = 16K
	mem2		: ORIGIN = 0x13f00000, LENGTH = 1M
}

__mem2_area_start = ORIGIN(mem2);
__mem2_area_size = LENGTH(mem2);
__mem2_area_end = ORIGIN(mem2) + LENGTH(mem2);

__page_table = ORIGIN(pagetable);

SECTIONS
{
	.rodata.mem2 :
	{
		*(.rodata.mem2)
		. = ALIGN(4);
	} >mem2

	.data.mem2 :
	{
		*(.data.mem2)
		. = ALIGN(4);
	} >mem2

	.bss.mem2 :
	{
		__bss2_start = . ;
		*(.bss.mem2)
		. = ALIGN(4);
		__bss2_end = . ;
	} >mem2
	
	.ipcinfo __mem2_area_end - 4 :
	{
		LONG(__ipc_info);
	} >mem2	

	.init :
	{
		*(.init)
		. = ALIGN(4);
	} >sram

	.text :
	{
		*(.text*)
		*(.text.*)
		*(.gnu.warning)
		*(.gnu.linkonce.t*)
		*(.glue_7)
		*(.glue_7t)
		. = ALIGN(4);
	} >sram

	.rodata :
	{
		*(.rodata)
		*all.rodata*(*)
		*(.roda)
		*(.rodata.*)
		*(.gnu.linkonce.r*)
		. = ALIGN(4);
	} >sram2

	.data :
	{
		*(.data)
		*(.data.*)
		*(.gnu.linkonce.d*)
		. = ALIGN(4);
	} >sram2

	.bss :
	{
		__bss_start = . ;
		*(.dynbss)
		*(.gnu.linkonce.b*)
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = . ;
	} >sram2

	.stack :
	{
		. = ALIGN(16);
		__stack_end = .;
		. = . +__stack_size;
		. = ALIGN(16);
		__stack_addr = .;
		__irqstack_end = .;
		. = . +__irqstack_size;
		. = ALIGN(16);
		__irqstack_addr = .;
		__excstack_end = .;
		. = . +__excstack_size;
		. = ALIGN(16);
		__excstack_addr = .;
	} >sram2

	/DISCARD/ :
	{
		*(.ARM.exidx*)
		*(.ARM.extab*)
	}

}


