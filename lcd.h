#ifndef __LCD_H__
#define __LCD_H__

#include "types.h"

void lcd_init(void);
int lcd_putchar(int c);
int lcd_puts(const char *s);
void lcd_setdelay(u32 delay);
int lcd_printf( const char *fmt, ...);
#endif
