#include <stdint.h>
#include <stddef.h>

#include "kterm.h"
#include "boot.h"
#include "bmft.h"
#include "stdarg.h"

static struct cursor {
    uint32_t* screen;
    uint16_t x, y, // Top left of the cursor
        width, height, // Size of the cursor, also size of a char
        s_pitch,
        s_height,
        s_width;
} cursor;
static Bmft         *font;
static Framebuffer  *fb;

int kterminit(BootInfo* bootInfo)
{
    font = (Bmft *)bootInfo->files->files[1].data;
    if (font->magicNumber != BMFT_MAGIC)
        return -1;
    if (font->version != BMFT_VERSION)
        return -2;

    fb = bootInfo->frameBuffer;
    cursor = (struct cursor){
        .screen = (uint32_t*)fb->addr,
        .x = 0,
        .y = 0,
        .width = 8,
        .height = 12,
        .s_pitch = fb->pitch,
        .s_height = fb->height,
        .s_width = fb->width
    };

    return 0;
}

inline static void kputpixel(uint32_t color, uint16_t x, uint16_t y)
{
    cursor.screen[y*cursor.s_pitch + x] = color;
}

void kfillscreen(uint32_t color)
{

    for (uint16_t i = 0; i < cursor.s_height; i++)
    {
        for (uint16_t j = 0; j < cursor.s_width; j++)
        {
            kputpixel(color, j, i);
        }
    }
    cursor.x = 0;
    cursor.y = 0;
}

void kclearline() {
    for (uint16_t y = cursor.y; y < cursor.y + cursor.height; y++)
    {
        for (uint16_t x = 0; x < cursor.s_width; x++)
        {
            kputpixel(0xFF000000, x, y);
        }
    }
}

void knewline()
{
    cursor.x = 0;
    // if next block can't fit vertically, scroll
    if (cursor.y + 2 * cursor.height > cursor.s_height) {
        uint32_t *base = (uint32_t *)fb->addr;
        for (uint64_t i = 0; i < fb->height - cursor.height; i++) {
            for (uint64_t j = 0; j < fb->width; j++) {
                uint32_t *addr = base + fb->width * i + j;
                *(addr) = *(addr + fb->width * cursor.height);
            }
        }
        kclearline();
    }
    else cursor.y += cursor.height;
}

void kputc(unsigned char chr)
{
    if (chr < PRINTABLE_ASCII_FIRST) {
        switch (chr)
        {
        case '\n':  // new line
            knewline();
            break;
        case '\r':  // carriage return
            cursor.x = 0;
            break;
        case '\t':  // horz tab
            cursor.x += 4 * cursor.width;
            break;
        case '\b':  // backspace
            cursor.x -= cursor.width;
            break;
        case '\a':  // bell
            kfillscreen(0xFFFF0000);
            break;
        default:
            cursor.x += cursor.width;
            break;
        }
    } else {
        if (cursor.x + cursor.width > cursor.s_width) knewline();

        for (uint32_t x = 0; x < cursor.width; x++)
            for (uint32_t y = 0; y < cursor.height; y++)
                if (font->glpyhs[chr - PRINTABLE_ASCII_FIRST].px12[y] & (1<<x))
                    kputpixel(0xFFFFFFFF, cursor.x + x, cursor.y + y);
        cursor.x += cursor.width;
    }
}

void kputs(const char* str)
{
    while (*str) {
        kputc(*(str++));
    }
}


static char tmp[64];
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
static inline void kprintlong(int64_t n) {
    if (n<0) {
        kputc('-');
        n *= -1;
    }
    kprintulong(n);
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
