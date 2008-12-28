#ifndef __PANIC_H__
#define __PANIC_H__

#define PANIC_MOUNT 1,1,-1

void panic2(int mode, ...)  __attribute__ ((noreturn));

#endif
