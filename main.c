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

u32 match[] = {
	0xF7FFFFB8,
	0xBC024708,
	1,
	2,
};

static inline void mem1_poke(u8 *dst, u8 bv)
{
	u32 *p = (u32*)(((u32)dst) & ~3);
	u32 val = *p;
	
	switch(((u32)dst) & 3) {
		case 0:
			val = (val & 0x00FFFFFF) | bv << 24;
			break;
		case 1:
			val = (val & 0xFF00FFFF) | bv << 16;
			break;
		case 2:
			val = (val & 0xFFFF00FF) | bv << 8;
			break;
		case 3:
			val = (val & 0xFFFFFF00) | bv;
			break;
	}
	
	*p = val;
}

void *memcpy_mem1(void *dst, const void *src, size_t n)
{
	unsigned char *p;
	const unsigned char *q;

	for (p = dst, q = src; n; n--) {
		mem1_poke(p++, *q++);
	}

	return dst;
}

static void patch_mem(u8 *offset, u32 size, u64 titleID)
{
	while(size>sizeof(match)) {
		if(!memcmp(offset, match, sizeof(match))) {
			gecko_printf("--> Patching @ %p\n", offset+8);
			gecko_printf("--> To TitleID %08x%08x\n", (u32)(titleID>>32), (u32)titleID);
			memcpy_mem1(offset+8, &titleID, sizeof(u64));
			return;
		}
		offset++;
		size -= 1;
	}
}

void boot2_patchelf(u8 *elf, u64 titleID) {
	gecko_puts("Patching boot2 ELF...\n");

	if(memcmp("\x7F" "ELF\x01\x02\x01\x61\x01",elf,9)) {
		gecko_printf("Invalid ELF header! 0x%02x 0x%02x 0x%02x 0x%02x\n",elf[0], elf[1], elf[2], elf[3]);
		return;
	}

	Elf32_Ehdr *ehdr = (Elf32_Ehdr*)elf;
	if(ehdr->e_phoff == 0) {
		gecko_printf("ELF has no program headers!\n");
		return;
	}
	int count = ehdr->e_phnum;
	Elf32_Phdr *phdr = (Elf32_Phdr*)(elf + ehdr->e_phoff);
	gecko_printf("PHDRS at %p\n",phdr);
	while(count--)
	{
		if(phdr->p_type != PT_LOAD) {
			gecko_printf("Skipping PHDR of type %d\n",phdr->p_type);
		} else {
			void *src = elf + phdr->p_offset;
			gecko_printf("PATCH %p -> %p/%p [0x%x]\n",src, phdr->p_paddr, phdr->p_vaddr, phdr->p_filesz);
			if(phdr->p_vaddr == (void*)0x20100000) {
				gecko_printf("-> Found ES code PHDR\n");
				patch_mem(src, phdr->p_filesz, titleID);
			}
		}
		phdr++;
	}
	gecko_puts("Done!\n");
}

#define PPC_BOOT_FILE "/system/ppcboot.elf"

FATFS fatfs;

void *patch_boot2(void *base, u64 titleID)
{

	ioshdr *hdr = (ioshdr*)base;
	ioshdr *parhdr = (ioshdr*)hdr->argument;
	u8 *elf;

	gecko_puts("Patching BOOT2 for leet hax\n");
	gecko_printf("Parent BOOT2 header (@%p):\n",parhdr);
	gecko_printf(" Header size: %08x\n", parhdr->hdrsize);
	gecko_printf(" Loader size: %08x\n", parhdr->loadersize);
	gecko_printf(" ELF size: %08x\n", parhdr->elfsize);
	gecko_printf(" Argument: %08x\n", parhdr->argument);

	elf = (u8*) parhdr;
	elf += parhdr->hdrsize + parhdr->loadersize;
	gecko_printf("ELF at %p\n",elf);

	boot2_patchelf(elf, titleID);

	parhdr->argument = 0x42;
	
	gecko_printf("Vector: %p\n", (void*)(((u32)parhdr) + parhdr->hdrsize));

	return (void*)(((u32)parhdr) + parhdr->hdrsize);
}

void *_main(void *base)
{
	FRESULT fres;
	int res;

	gecko_init();
	gecko_puts("MiniIOS v0.1 loading\n");

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

	u32 tidh=0, tidl;

	while(1) {
		if(read32(HW_IPC_ARMCTRL) & IPC_CTRL_RECV) {
			tidh = read32(HW_IPC_PPCMSG);
			write32(HW_IPC_ARMCTRL, read32(HW_IPC_ARMCTRL) | IPC_CTRL_RECV);
		}
		if(read32(HW_IPC_ARMCTRL) & IPC_CTRL_SENT) {
			tidl = read32(HW_IPC_PPCMSG);
			write32(HW_IPC_ARMCTRL, read32(HW_IPC_ARMCTRL) | IPC_CTRL_SENT);
			break;
		}
	}

	gecko_puts("Returning to BootMii...\n");

	return patch_boot2(base, (((u64)tidh)<<32) | tidl);
}
