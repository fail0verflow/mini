#ifndef	__POWERPC_ELF_H__
#define	__POWERPC_ELF_H__	1

int powerpc_boot_file(const char *path);
int powerpc_boot_mem(const u8 *addr, u32 len);

#endif
