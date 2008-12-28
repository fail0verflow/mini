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
#include "lcd.h"

void hexline(void *addr, int len)
{
	u8 *p = (u8*)addr;
	while(len--) {
		gecko_printf("%02x",*p++);
	}
}

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

#define BOOT_FILE "/system/iosboot.bin"

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

void hex32(u32 x) {
	int i;
	u8 b;
	for(i=0;i<8;i++) {
		b = x >> 28;
		if(b > 9)
			lcd_putchar(b-10+'a');
		else
			lcd_putchar(b+'0');
		x <<= 4;
	}
}

extern void *__end;

void *_main(void *base)
{
	FRESULT fres;
	
	ioshdr *hdr = (ioshdr*)base;
	ioshdr *iosboot;
	u8 *elf;
	
	elf = (u8*) base;
	elf += hdr->hdrsize + hdr->loadersize;
	
	dogecko = 1;
	
	debug_output(0xF1);
	mem_setswap(1);
	
	write32(HW_IRQENABLE, 0);
	
	clear32(HW_GPIO1DIR, 0x80);
	clear32(HW_GPIO1OWNER, 0x80);
	udelay(10000);
	if(read32(HW_GPIO1IN) & 0x80) {
		dogecko = 0;
		goto boot2;
	}
	
	// NOTE: END DEBUG CRITICAL ZONE
	
	lcd_init();
	
	regs_setup();
	//debug_output(0x50);
	//debug_output(0x51);
	debug_output(0xF8);
	
	gecko_init();
	
	debug_output(0xF9);

	lcd_puts("BootMii v0.1\n");

	if(dogecko) {
		gecko_puts("Hello, world from Starlet again!\n");
		gecko_puts("BootMii here, version 0.1\n");
		gecko_printf("BOOT2 header (@%p):\n",hdr);
		gecko_printf(" Header size: %08x\n", hdr->hdrsize);
		gecko_printf(" Loader size: %08x\n", hdr->loadersize);
		gecko_printf(" ELF size: %08x\n", hdr->elfsize);
		gecko_printf(" Argument: %08x\n", hdr->argument);
		
		gecko_printf("ELF at %p\n",elf);
		
		gecko_puts("Trying to mount SD...\n");
	}
	
	fres = f_mount(0, &fatfs);
	
	if(fres != FR_OK) {
		if(dogecko)
			gecko_printf("Error %d while trying to mount SD\n", fres);
		debug_output(0x12);
		debug_output(fres&0xFF);
		goto boot2;
	}
	
	//debug_output(0xF2);
	
	if(dogecko)
		gecko_puts("Trying to open SD:" BOOT_FILE "\n");
	
	FIL fd;
	u32 read;
	
	fres = f_open(&fd, BOOT_FILE, FA_READ);
	if(fres != FR_OK) {
		if(dogecko)
			gecko_printf("Error %d while trying to open file\n", fres);
		//debug_output(0x13);
		//debug_output(fres&0xFF);
		goto boot2;
	}
	
	lcd_puts(".");
	
	// NOTE: END CRITICAL ZONE
	// anything from here to boot2: shouldn't be able to cause a brick
	
	debug_output(0xF2);
	
	iosboot = (ioshdr *)ALIGN_FORWARD(((u32)&__end) + 0x100, 0x100);
	
	if(dogecko)
		gecko_printf("Trying to read IOSBOOT header to %p\n", iosboot);
	
	fres = f_read(&fd, iosboot, sizeof(ioshdr), &read);
	if(fres != FR_OK) {
		if(dogecko)
			gecko_printf("Error %d while trying to read file header\n", fres);
		//debug_output(0x14);
		//debug_output(fres&0xFF);
		goto boot2;
	}
	if(read != sizeof(ioshdr)) {
		if(dogecko)
			gecko_printf("Got %d bytes, expected %d\n", read, sizeof(ioshdr));
		//debug_output(0x24);
		goto boot2;
	}
	
	lcd_puts(".");
	//debug_output(0xF5);
	
	if(dogecko) {
		gecko_printf("IOSBOOT header (@%p):\n",iosboot);
		gecko_printf(" Header size: %08x\n", iosboot->hdrsize);
		gecko_printf(" Loader size: %08x\n", iosboot->loadersize);
		gecko_printf(" ELF size: %08x\n", iosboot->elfsize);
		gecko_printf(" Argument: %08x\n", iosboot->argument);
	}
	
	u32 totalsize = iosboot->hdrsize + iosboot->loadersize + iosboot->elfsize;
	
	if(dogecko) {
		gecko_printf("Total IOSBOOT size: 0x%x\n", totalsize);
		
		gecko_printf("Trying to read IOSBOOT to %p\n", iosboot);
	}
	
	fres = f_read(&fd, iosboot+1, totalsize-sizeof(ioshdr), &read);
	if(fres != FR_OK) {
		if(dogecko)
			gecko_printf("Error %d while trying to read file header\n", fres);
		//debug_output(0x15);
		//debug_output(fres&0xFF);
		goto boot2;
	}
	if(read != (totalsize-sizeof(ioshdr))) {
		if(dogecko)
			gecko_printf("Got %d bytes, expected %d\n", read, (totalsize-sizeof(ioshdr)));
		//debug_output(0x25);
		goto boot2;
	}
	
	lcd_puts(".");
	
	//debug_output(0xF6);
	
	if(dogecko) {
		gecko_puts("IOSBOOT read\n");
		gecko_printf("Setting argument to %p\n", base);
	}
	iosboot->argument = (u32)base;
	
	void *entry = (void*)(((u32)iosboot) + iosboot->hdrsize);
	
	lcd_puts(" \x7e IOSBOOT \n");
	
	if(dogecko)
		gecko_printf("Launching IOSBOOT @ %p\n",entry);
	
	debug_output(0xF3);
	
	return entry;

boot2:
	debug_output(0xC8);
	if(dogecko)
		gecko_puts("Couldn't load from SD, falling back to boot2\n");
	lcd_puts(" \x7e BOOT2 \n");
	boot2_loadelf(elf);
	debug_output(0xC9);
	return (void *) 0xFFFF0000;

}
