#include "types.h"
#include "utils.h"
#include "start.h"
#include "hollywood.h"
#include "sdhc.h"
#include "string.h"
#include "memory.h"
#include "elf.h"
#include "gecko.h"
#include "ff.h"
#include "panic.h"
#include "powerpc_elf.h"

typedef struct {
	u32 hdrsize;
	u32 loadersize;
	u32 elfsize;
	u32 argument;
} ioshdr;

int dogecko;

void boot2_loadelf(u8 *elf) {
	if(dogecko)
		gecko_puts("Loading boot2 ELF...\n");

	if(memcmp("\x7F" "ELF\x01\x02\x01\x61\x01",elf,9)) {
		if(dogecko)
			gecko_printf("Invalid ELF header! 0x%02x 0x%02x 0x%02x 0x%02x\n",elf[0], elf[1], elf[2], elf[3]);
		panic(0xE3);
	}
	
	Elf32_Ehdr *ehdr = (Elf32_Ehdr*)elf;
	if(ehdr->e_phoff == 0) {
		if(dogecko)
			gecko_printf("ELF has no program headers!\n");
		panic(0xE4);
	}
	int count = ehdr->e_phnum;
	Elf32_Phdr *phdr = (Elf32_Phdr*)(elf + ehdr->e_phoff);
	if(dogecko)
		gecko_printf("PHDRS at %p\n",phdr);
	while(count--)
	{
		if(phdr->p_type != PT_LOAD) {
			if(dogecko)
				gecko_printf("Skipping PHDR of type %d\n",phdr->p_type);
		} else {
			void *src = elf + phdr->p_offset;
			if(dogecko)
				gecko_printf("LOAD %p -> %p [0x%x]\n",src, phdr->p_paddr, phdr->p_filesz);
			memcpy(phdr->p_paddr, src, phdr->p_filesz);
		}
		phdr++;
	}
	if(dogecko)
		gecko_puts("Done!\n");
}

#define PPC_BOOT_FILE "/system/ppcboot.elf"

FATFS fatfs;

void turn_stuff_on(void)
{
	clear32(HW_GPIO1OUT, 0x10);
	udelay(100);
	set32(HW_RESETS, 0x7FFFFCF);
}

void reset_audio(u8 flag)
{

	// GPIO2IN is probably mislabeled
	if(flag)
		clear32(HW_DIFLAGS, 0x180);
	else
		mask32(HW_DIFLAGS, 0x80, 0x100);
	
	clear32(HW_GPIO2IN, 0x80000000);
	udelay(2);
	clear32(HW_GPIO2IN, 0x40000000);
	
	if(flag) {
		clear32(HW_GPIO2IN, 0x10000000);
		mask32(HW_GPIO2DIR, 0x7FFFFFF, 0x4B0FFCE);
	} else {
		mask32(HW_GPIO2DIR, 0x7FFFFFF, 0x4640FC0);
	}
	udelay(10);
	set32(HW_GPIO2IN, 0x40000000);
	udelay(500);
	set32(HW_GPIO2IN, 0x80000000);
	udelay(2);
}

void regs_setup(void)
{
	u8 hwood_ver, hwood_hi, hwood_lo;
	hwood_ver = read32(0xd800214);
	hwood_hi = hwood_ver >> 4; //R0
	hwood_lo = hwood_ver & 0xF; //R1
	
	*(u32*)0xFFFF897C = *(u32*)0xFFFF86D0;
	set32(HW_EXICTRL, EXICTRL_ENABLE_EXI);
	mem_protect(1, (void*)0x13420000, (void*)0x1fffffff);
	clear32(HW_EXICTRL, 0x10);
	if(hwood_hi == 0)
		write32(0xd8b0010, 0);
	write32(0xd8b0010, 0);
	if(hwood_hi == 1 && hwood_lo == 0)
		mask32(0xd800140, 0x0000FFF0, 1);
	set32(0xd80018C, 0x400);
	set32(0xd80018C, 0x800);
	
	reset_audio(0);
	//boot2_sub_FFFF5D08(0);
	//boot2_sub_FFFF5C40(hwood_hi);
	//boot2_sub_FFFF6AA8();
	
	turn_stuff_on();
	// what do these two pokes do? no clue. Not needed but I'm leaving them in anyway.
	write32(0xd8001e0, 0x65244A); //?
	write32(0xd8001e4, 0x46A024); //?
	
	clear32(HW_GPIO1OWNER, 0x10);
	set32(HW_GPIO1DIR, 0x10);
	//write32(HW_ALARM,0);
	//write32(HW_ALARM,0);
}

void *_main(void *base)
{
	FRESULT fres;
	int res;
	
	mem_setswap(1);
	
	write32(HW_IRQENABLE, 0);
	
	regs_setup();
	//debug_output(0x50);
	//debug_output(0x51);
	debug_output(0xF8);
	
	gecko_init();
	
	debug_output(0xF9);

	gecko_puts("MiniIOS v0.1 loading\n");

	fres = f_mount(0, &fatfs);
	
	if(fres != FR_OK) {
		gecko_printf("Error %d while trying to mount SD\n", fres);
		panic2(0, PANIC_MOUNT);
	}
	
	gecko_puts("Trying to boot:" PPC_BOOT_FILE "\n");
	
	res = powerpc_load_file(PPC_BOOT_FILE);
	if(res < 0) {
		gecko_printf("Failed to boot PPC: %d\n", res);
		gecko_puts("Continuing anyway\n");
	}

	while(1);
}
