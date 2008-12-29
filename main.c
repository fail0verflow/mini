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

int dogecko = 1;

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

void boot2_init1() { //func_ffff5d08
        u32 reg = 0xd8001c8;

        if((s32) read32(reg) < 0)
                return;
        clear32(reg, 0x80000000);
        udelay( 2);
        clear32(reg, 0x40000000);
        udelay(10);
        set32(reg,   0x40000000);
        udelay(50);
        set32(reg,   0x80000000);
        udelay( 2);
}

void boot2_init2(u32 hlywdVerHi) { //func_ffff5c40
        u32 reg = 0xD800088;
        write32(reg, 0xFE);
        udelay(2);
        clear32(0xD80001D8, 0x80000000);
        udelay(2);
        clear32(0xD80001D8, 0x40000000);
        udelay(10);
        set32(0xD80001D8, 0x40000000);
        udelay(50);
        set32(0xD80001DB, 0x80000000);
        udelay(2);
        write32(reg, 0xF6);
        udelay(50);
        write32(reg, 0xF4);
        udelay(1);
        write32(reg, 0xF0);
        udelay(1);
        write32(reg, 0x70);
        udelay(1);
        write32(reg, 0x60);
        udelay(1);
        write32(reg, 0x40);
        udelay(1);
        write32(reg, 0);
        udelay(1);
        write32(0xD0400B4, 0x2214);
        if (hlywdVerHi)
                write32(0xD0400B0, 0x20600);
        else
                write32(0xD0400B0, 0x20400);
        write32(0xD0400A4, 0x26);
        udelay(1);
        write32(0xD0400A4, 0x2026);
        udelay(1);
        write32(0xD0400A4, 0x4026);
        udelay(20);
        write32(0xD0400CC, 0x111);
}

void setup_gpios()
{
        //do this later
}
void (*boot2_setup_audio)(u32) = (void*)0xFFFF5E29;
void regs_setup(void)
{
        u8 hwood_ver, hwood_hi, hwood_lo;
        hwood_ver = read32(0xd800214)  & 0xFF;
        hwood_hi = hwood_ver >> 4; //R0
        hwood_lo = hwood_ver & 0xF; //R1

        write32(0xFFFF897C, read32(0xFFFF86D0));
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

        //double check this to see if we can fix buzzing
	//reset_audio(0);
        boot2_setup_audio(0);
	boot2_init1();
        boot2_init2(hwood_hi);
        setup_gpios();

        turn_stuff_on();
        // what do these two pokes do? no clue. Not needed but I'm leaving them in anyway.
        write32(0xd8001e0, 0x65244A); //?
        write32(0xd8001e4, 0x46A024); //?

        clear32(HW_GPIO1OWNER, 0x10);
        set32(HW_GPIO1DIR, 0x10);
        //write32(HW_ALARM,0);
        //write32(HW_ALARM,0);
}

void load_boot2(void *base)
{

	ioshdr *hdr = (ioshdr*)base;
	ioshdr *parhdr = (ioshdr*)hdr->argument;
	u8 *elf;

	gecko_puts("Loading BOOT2 for leet hax\n");
	gecko_printf("Parent BOOT2 header (@%p):\n",parhdr);
	gecko_printf(" Header size: %08x\n", parhdr->hdrsize);
	gecko_printf(" Loader size: %08x\n", parhdr->loadersize);
	gecko_printf(" ELF size: %08x\n", parhdr->elfsize);
	gecko_printf(" Argument: %08x\n", parhdr->argument);
	
	elf = (u8*) parhdr;
	elf += parhdr->hdrsize + parhdr->loadersize;
	gecko_printf("ELF at %p\n",elf);
	
	boot2_loadelf(elf);
}

void (*boot2_setup_memory)(void) = (void*)0xFFFF1EAD;
void *_main(void *base)
{
	FRESULT fres;
	int res;
	
	gecko_init();
	printf("In bootmii arm\n");	
	mem_setswap(1);
	
	write32(HW_IRQENABLE, 0);
	
	regs_setup();
	//debug_output(0x50);
	//debug_output(0x51);
	debug_output(0xF8);
	
	//gecko_init();
	
	debug_output(0xF9);

	gecko_puts("MiniIOS v0.1 loading\n");
	
	load_boot2(base);
	
	write32(0xFFFF1F60,0);	
	
	gecko_puts("Setting up hardware\n");
	
	//boot2_setup_memory();
	//regs_setup();	
	gecko_puts("Done hardware setup\n");

	write32(0xFFFF1F60,0xF002FB74);	

	gecko_printf("GPIO1OUT %08x\n",read32(HW_GPIO1OUT));
	gecko_printf("GPIO1DIR %08x\n",read32(HW_GPIO1DIR));
	gecko_printf("GPIO1IN  %08x\n",read32(HW_GPIO1IN));
	gecko_printf("GPIO1OWN %08x\n",read32(HW_GPIO1OWNER));

/*
// Starlet side of GPIO1
// Output state
#define		HW_GPIO1OUT			(HW_REG_BASE + 0x0e0)
// Direction (1=output)
#define		HW_GPIO1DIR			(HW_REG_BASE + 0x0e4)
// Input state
#define		HW_GPIO1IN			(HW_REG_BASE + 0x0e8)
// Interrupt level
#define		HW_GPIO1INTLVL		(HW_REG_BASE + 0x0ec)
// Interrupt flags (write 1 to clear)
#define		HW_GPIO1INTFLAG		(HW_REG_BASE + 0x0f0)
// Interrupt propagation enable (interrupts go to main interrupt 0x800)
#define		HW_GPIO1INTENABLE	(HW_REG_BASE + 0x0f4)
//??? seems to be a mirror of inputs at some point... power-up state?
#define		HW_GPIO1INMIR		(HW_REG_BASE + 0x0f8)
// Owner of each GPIO bit. If 1, GPIO1B registers assume control. If 0, GPIO1 registers assume control.
#define		HW_GPIO1OWNER		(HW_REG_BASE + 0x0fc)
*/

	fres = f_mount(0, &fatfs);
	
	if(fres != FR_OK) {
		gecko_printf("Error %d while trying to mount SD\n", fres);
		panic2(0, PANIC_MOUNT);
	}
	
	gecko_puts("Trying to boot:" PPC_BOOT_FILE "\n");
	
	write32(HW_IPC_PPCMSG, 0);
	write32(HW_IPC_ARMMSG, 0);
	write32(HW_IPC_PPCCTRL, IPC_CTRL_SENT|IPC_CTRL_RECV);
	write32(HW_IPC_ARMCTRL, IPC_CTRL_SENT|IPC_CTRL_RECV);
	
	res = powerpc_load_file(PPC_BOOT_FILE);
	if(res < 0) {
		gecko_printf("Failed to boot PPC: %d\n", res);
		gecko_puts("Continuing anyway\n");
	}

	while(1) {
		u32 tidh, tidl;
		if(read32(HW_IPC_ARMCTRL) & IPC_CTRL_RECV) {
			gecko_puts("STARLET ping1\n");
			tidh = read32(HW_IPC_PPCMSG);
			gecko_printf("TIDH = %08x\n",tidh);
			//load_boot2(base);
			write32(0x135c0d20, tidh);
			gecko_puts("TIDH written\n");
			write32(HW_IPC_ARMCTRL, read32(HW_IPC_ARMCTRL) | IPC_CTRL_RECV);
			gecko_puts("STARLET ping1 end\n");
		}
		if(read32(HW_IPC_ARMCTRL) & IPC_CTRL_SENT) {
			gecko_puts("STARLET ping2\n");
			tidl = read32(HW_IPC_PPCMSG);
			gecko_printf("TIDL = %08x\n",tidl);
			//load_boot2(base);
			write32(0x135c0d24, tidl);
			gecko_puts("TIDL written\n");
			write32(HW_IPC_ARMCTRL, read32(HW_IPC_ARMCTRL) | IPC_CTRL_SENT);
			gecko_puts("STARLET ping2 end\n");
			break;
		}
	}

	return (void *) 0xFFFF0000;
}
