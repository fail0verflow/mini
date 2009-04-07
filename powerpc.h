#ifndef	__POWERPC_H__
#define	__POWERPC_H__	1

void ppc_boot_code();
void powerpc_upload_stub(u32 entry);
void powerpc_hang();
void powerpc_reset();

#endif
