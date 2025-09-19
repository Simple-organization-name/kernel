#ifndef __VISU_H__
#define __VISU_H__

#include "boot.h"

void init_visu(BootInfo *bootInfo, uint16_t size);
void fill_screen(uint32_t color);
void new_line();
void log_color(uint32_t color);

void kputc(unsigned char chr);
void kputs(const char *str);
void kprintf(const char *format, ...);

#endif