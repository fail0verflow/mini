OUTPUT_FORMAT("elf32-bigarm")
OUTPUT_ARCH(arm)
EXTERN(_start)
ENTRY(_start)

__base_addr	=	0xffff0000;

__data_addr =   0x11000000;

__stack_area =  0xfffe0000;

SECTIONS
{
	. = __base_addr;

	.init :
	{
		*(.init)
		. = ALIGN(4);
	}

	.text :
	{
		*(.text.*)
		*(.gnu.warning)
		*(.gnu.linkonce.t*)
		*(.glue_7)
		*(.glue_7t)
		. = ALIGN(4);
	}

	__text_end = . ;
	
	. = __data_addr;

	.rodata :
	{
		*(.rodata)
		*all.rodata*(*)
		*(.roda)
		*(.rodata.*)
		*(.gnu.linkonce.r*)
		. = ALIGN(4);
	}
	
	.data :
	{
		*(.data)
		*(.data.*)
		*(.gnu.linkonce.d*)
		. = ALIGN(4);
	}
	
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
	}

	. = __stack_area;
	.stack :
	{
		__stack_end = .;
		. += 0x800;
		LONG(0);
		. = ALIGN(64);
		__stack_addr = .;
	}
	
	__end = .;
	
}

PROVIDE (__stack_end = __stack_end);
PROVIDE (__stack_addr = __stack_addr);
PROVIDE (__bss_start = __bss_start);
PROVIDE (__bss_end = __bss_end);
