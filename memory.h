#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "types.h"

#define ALIGN_FORWARD(x,align) \
	((typeof(x))((((u32)(x)) + (align) - 1) & (~(align-1))))

#define ALIGN_BACKWARD(x,align) \
	((typeof(x))(((u32)(x)) & (~(align-1))))

enum AHBDEV {
	MEMORY = 0,
	RAW0 = 0x100,
	RAWF = 0x10F,
};

void dc_flushrange(void *start, u32 size);
void dc_invalidaterange(void *start, u32 size);
void dc_flushall(void);
void ic_invalidateall(void);
void magic_bullshit(int type);
void ahb_memflush(enum AHBDEV dev);
void mem_protect(int enable, void *start, void *end);
void mem_setswap(int enable);

#endif
