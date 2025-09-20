#ifndef __KTERM_H__
#define __KTERM_H__

#include "boot.h"

int kterminit(BootInfo *bootInfo);
void kfillscreen(uint32_t color);
void knewline();
void kclearline();

void kputc(unsigned char chr);
void kputs(const char *str);
void kprintf(const char *format, ...);

#endif
