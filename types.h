#ifndef __TYPES_H__
#define __TYPES_H__

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef volatile unsigned char vu8;
typedef volatile unsigned short vu16;
typedef volatile unsigned int vu32;
typedef volatile unsigned long long vu64;

typedef volatile signed char vs8;
typedef volatile signed short vs16;
typedef volatile signed int vs32;
typedef volatile signed long long vs64;

typedef s32 size_t;

#define NULL ((void *)0)

#define MEM2_BSS __attribute__ ((section (".bss.mem2")))
#define MEM2_DATA __attribute__ ((section (".data.mem2")))
#define MEM2_RODATA __attribute__ ((section (".rodata.mem2")))
#define ALIGNED(x) __attribute__((aligned(x)))

#define STACK_ALIGN(type, name, cnt, alignment)         \
	u8 _al__##name[((sizeof(type)*(cnt)) + (alignment) + \
	(((sizeof(type)*(cnt))%(alignment)) > 0 ? ((alignment) - \
	((sizeof(type)*(cnt))%(alignment))) : 0))]; \
	type *name = (type*)(((u32)(_al__##name)) + ((alignment) - (( \
	(u32)(_al__##name))&((alignment)-1))))


#define INT_MAX ((int)0x7fffffff)
#define UINT_MAX ((unsigned int)0xffffffff)

#endif
