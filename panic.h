#ifndef __PANIC_H__
#define __PANIC_H__

#define PANIC_MOUNT      1,1,-1
#define PANIC_EXCEPTION  1,3,1,-1
#define PANIC_IPCOVF     1,3,3,-1

void panic2(int mode, ...)  __attribute__ ((noreturn));

#endif
