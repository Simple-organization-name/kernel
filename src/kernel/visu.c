#include <stdint.h>
#include <stddef.h>

#include "visu.h"
#include "boot.h"
#include "bmft.h"
#include "stdarg.h"

static struct cursor {
    uint32_t* screen;
    uint16_t x, y,
        size,
        s_pitch,
        s_height,
        s_width;
} where;
static Bmft *font;

void init_visu(BootInfo* bootInfo, uint16_t size)
{
    font = (Bmft *)bootInfo->files->files[1].data;

    Framebuffer *fb = bootInfo->frameBuffer;
    where = (struct cursor){
        .screen = (uint32_t*)fb->addr,
        .x = 0,
        .y = 0,
        .size = size,
        .s_pitch = fb->pitch,
        .s_height = fb->height,
        .s_width = fb->width
    };
}

inline static void put_pixel(uint32_t color, uint16_t x, uint16_t y)
{
    where.screen[y*where.s_pitch + x] = color;
}

void fill_screen(uint32_t color)
{
    
    for (uint16_t i = 0; i < where.s_height; i++)
    {
        for (uint16_t j = 0; j < where.s_width; j++)
        {
            put_pixel(color, j, i);
        }
    }
    where.x = 0;
    where.y = 0;
}

void new_line()
{
    where.x = 0;
    where.y += 12;
    // if next block can't fit vertically, wrap around
    if (where.y + 12 > where.s_height)
        where.y = 0;
    // clear the line just in case junk is left over
    // for (uint16_t y = where.y; y < where.y + where.size; y++)
    // {
    //     for (uint16_t x = 0; x < where.s_width; x++)
    //     {
    //         put_pixel(0xFF000000, x, y);
    //     }
    // }
}

void log_color(uint32_t color)
{
    if (where.x + where.size > where.s_width)
    new_line();
    
    for (uint16_t i = where.y; i < where.y + where.size; i++)
    {
        for (uint16_t j = where.x; j < where.x + where.size; j++)
        {
            put_pixel(color, j, i);
        }
    }
    where.x += where.size;
}

void kputc(unsigned char chr)
{
    if (chr < 32) {
        switch (chr)
        {
        case '\n':  // new line
            new_line();
            break;
        case '\r':  // carriage return
            where.x = 0;
            break;
        case '\t':  // horz tab
            where.x += 4*where.size;
            break;
        case '\b':  // backspace
            where.x -= 8;
            break;
        case '\a':  // bell
            fill_screen(0xFFFF0000);
            break;
        default:
            break;
        }
    } else {
        if (where.x + 8 > where.s_width) new_line();

        for (uint32_t x = 0; x < 8; x++)
            for (uint32_t y = 0; y < 12; y++)
                if (font->glpyhs[chr - PRINTABLE_ASCII_FIRST].px12[y] & (1<<x))
                    put_pixel(0xFFFFFFFF, where.x + x, where.y + y);
        where.x += 8;
    }
}

void kputs(const char* str)
{
    while (*str) {
        kputc(*(str++));
    }
}


static char tmp[64];
static inline void kprintlong(int64_t n) {
    if (n<0) {
        kputc('-');
        n *= -1;
    }
    int8_t i = 0;
    if (n == 0) kputc('0');
    else {
        while (n > 0) {
            tmp[i++] = '0' + n%10;
            n /= 10;
        }
        while (i-- > 0) kputc(tmp[i]);
    }
}
static inline void kprintulong(uint64_t n) {
    int8_t i = 0;
    if (n == 0) kputc('0');
    else {
        while (n > 0) {
            tmp[i++] = '0' + n%10;
            n /= 10;
        }
        while (i-- > 0) kputc(tmp[i]);
    }
}

void kprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    while (*format) {
        if (*format == '%') {
            switch (*(++format)) {
                case 's':
                    char *str = va_arg(args, char *);
                    kputs(str);
                    break;
                case 'd':
                    int n = va_arg(args, int);
                    kprintlong((long)n);
                    break;
                case 'D':
                    long ln = va_arg(args, long);
                    kprintlong(ln);
                    break;
                case 'u':
                    uint32_t u = va_arg(args, uint32_t);
                    kprintulong((uint64_t)u);
                    break;
                case 'U':
                    uint64_t lu = va_arg(args, uint64_t);
                    kprintulong((uint64_t)lu);
                    break;
                case 'c':
                    int c = va_arg(args, int);
                    kputc((uint8_t)c);
                    break;
                case 'l':
                    switch (*(++format)) {
                        case 'u':
                            uint64_t lu = va_arg(args, uint64_t);
                            kprintulong((uint64_t)lu);
                            break;
                        case 'd':
                            long ln = va_arg(args, long);
                            kprintlong(ln);
                            break;
                        default:
                            format--;
                            break;
                    }
                    break;
                case '%':
                    kputc('%');
                    break;
            }
        }
        else kputc(*format);
        format++;
    }
}

