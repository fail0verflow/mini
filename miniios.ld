OUTPUT_FORMAT("elf32-bigarm")
OUTPUT_ARCH(arm)
EXTERN(_start)
ENTRY(_start)

__stack_size = 0x800;
__irqstack_size = 0x100;

MEMORY {
	sram	: ORIGIN = 0xffff0000, LENGTH = 64K
	sram2	: ORIGIN = 0xfffe0000, LENGTH = 32K
	mem2	: ORIGIN = 0x13f00000, LENGTH = 1M
}

SECTIONS
{
	.rodata.ipc :
	{
		*(.rodata.ipc)
		. = ALIGN(4);
	} >mem2

	.bss.ipc :
	{
		*(.bss.ipc)
		. = ALIGN(4);
	} >mem2
	
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
		*(.sbss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = . ;
	} >sram2

	.stack :
	{
		__stack_end = .;
		. = . +__stack_size;
		. = ALIGN(64);
		__stack_addr = .;
		__irqstack_end = .;
		. = . +__irqstack_size;
		. = ALIGN(64);
		__irqstack_addr = .;
	} >sram2

}

PROVIDE (__stack_end = __stack_end);
PROVIDE (__stack_addr = __stack_addr);
PROVIDE (__irqstack_end = __irqstack_end);
PROVIDE (__irqstack_addr = __irqstack_addr);
PROVIDE (__bss_start = __bss_start);
PROVIDE (__bss_end = __bss_end);
PROVIDE (__bss2_start = __bss2_start);
PROVIDE (__bss2_end = __bss2_end);
