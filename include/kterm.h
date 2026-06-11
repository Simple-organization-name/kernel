#ifndef __KTERM_H__
#define __KTERM_H__

#include <boot/bootInfo.h>
#include <kerror.h>
#include <stdbool.h>

int kterminit(BootInfo *bootInfo, uint16_t ratio, bool hres);
void kputpixel(uint32_t color, uint16_t x, uint16_t y);
void kvputpixel(uint32_t color, uint16_t x, uint16_t y);
void ksetcursor(uint16_t x, uint16_t y);
void ksetvcursor(uint16_t x, uint16_t y);
void ksetcursorx(uint16_t x);
void ksetvcursorx(uint16_t x);
void ksetcursory(uint16_t y);
void ksetvcursory(uint16_t y);

void kfillscreen(uint32_t color);
void knewline();
void kclearline();

void kputc(unsigned char chr);
void kputs(const char *str);
void kprintf(const char *format, ...);

void kprintcheck(const char *str);

#endif
