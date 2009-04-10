/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

	PowerPC ELF file loading

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#include "types.h"
#include "powerpc.h"
#include "hollywood.h"
#include "utils.h"
#include "start.h"
#include "gecko.h"
#include "ff.h"
#include "powerpc_elf.h"
#include "elf.h"
#include "memory.h"
#include "string.h"

#define PHDR_MAX 10

static Elf32_Ehdr elfhdr;
static Elf32_Phdr phdrs[PHDR_MAX];
static FIL fd;

int powerpc_boot_file(const char *path)
{
	u32 read;
	FRESULT fres;
	gecko_printf("%s: %08x\n", __FUNCTION__, read32(HW_TIMER));
	fres = f_open(&fd, path, FA_READ);
	if(fres != FR_OK)
		return -fres;

	fres = f_read(&fd, &elfhdr, sizeof(elfhdr), &read);
	
	if(fres != FR_OK)
		return -fres;
	if(read != sizeof(elfhdr))
		return -100;

	if(memcmp("\x7F" "ELF\x01\x02\x01\x00\x00",elfhdr.e_ident,9)) {
		gecko_printf("Invalid ELF header! 0x%02x 0x%02x 0x%02x 0x%02x\n",elfhdr.e_ident[0], elfhdr.e_ident[1], elfhdr.e_ident[2], elfhdr.e_ident[3]);
		return -101;
	}

	if(elfhdr.e_phoff == 0 || elfhdr.e_phnum == 0) {
		gecko_printf("ELF has no program headers!\n");
		return -102;
	}
	if(elfhdr.e_phnum > PHDR_MAX) {
		gecko_printf("ELF has too many (%d) program headers!\n", elfhdr.e_phnum);
		return -102;
	}
	
	fres = f_lseek(&fd, elfhdr.e_phoff);
	if(fres != FR_OK)
		return -fres;

	fres = f_read(&fd, phdrs, sizeof(phdrs[0])*elfhdr.e_phnum, &read);
	if(fres != FR_OK)
		return -fres;
	if(read != sizeof(phdrs[0])*elfhdr.e_phnum)
		return -103;

	int count = elfhdr.e_phnum;
	Elf32_Phdr *phdr = phdrs;

	powerpc_hang();

	while(count--)
	{
		if(phdr->p_type != PT_LOAD) {
			gecko_printf("Skipping PHDR of type %d\n",phdr->p_type);
		} else {
			void *dst = phdr->p_paddr;

			gecko_printf("LOAD 0x%x -> %p [0x%x]\n", phdr->p_offset, phdr->p_paddr, phdr->p_filesz);
			fres = f_lseek(&fd, phdr->p_offset);
			if(fres != FR_OK)
				return -fres;
			fres = f_read(&fd, dst, phdr->p_filesz, &read);
			if(fres != FR_OK)
				return -fres;
			if(read != phdr->p_filesz)
				return -104;
		}
		phdr++;
	}

	dc_flushall();

	gecko_printf("ELF load done, booting PPC...\n");
	powerpc_upload_stub(elfhdr.e_entry);
	powerpc_reset();
	gecko_printf("PPC booted!\n");

	return 0;
}

int powerpc_boot_mem(const u8 *addr, u32 len)
{
	if (len < sizeof(Elf32_Ehdr))
		return -100;

	Elf32_Ehdr *ehdr = (Elf32_Ehdr *) addr;

	if (memcmp("\x7F" "ELF\x01\x02\x01\x00\x00", ehdr->e_ident, 9)) {
		gecko_printf("Invalid ELF header! 0x%02x 0x%02x 0x%02x 0x%02x\n",
						ehdr->e_ident[0], ehdr->e_ident[1],
						ehdr->e_ident[2], ehdr->e_ident[3]);
		return -101;
	}

	if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
		gecko_printf("ELF has no program headers!\n");
		return -102;
	}

	if (ehdr->e_phnum > PHDR_MAX) {
		gecko_printf("ELF has too many (%d) program headers!\n",
						elfhdr.e_phnum);
		return -102;
	}

	int count = ehdr->e_phnum;
	if (len < ehdr->e_phoff + count * sizeof(Elf32_Phdr))
		return -103;

	Elf32_Phdr *phdr = (Elf32_Phdr *) &addr[ehdr->e_phoff];

	// TODO: add more checks here
	// - phdrs out of bounds?
	// - loaded ELF overwrites itself?

	powerpc_hang();

	while (count--) {
		if (phdr->p_type != PT_LOAD) {
			gecko_printf("Skipping PHDR of type %d\n", phdr->p_type);
		} else {
			gecko_printf("LOAD 0x%x -> %p [0x%x]\n", phdr->p_offset, phdr->p_paddr, phdr->p_filesz);
			memcpy((void *) phdr->p_paddr, &addr[phdr->p_offset],
					phdr->p_filesz);
		}
		phdr++;
	}

	dc_flushall();

	gecko_printf("ELF load done, booting PPC...\n");
	powerpc_upload_stub(ehdr->e_entry);
	powerpc_reset();
	gecko_printf("PPC booted!\n");

	return 0;
}
