#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#include "kterm.h"
#include "boot/bootInfo.h"
#include "bmft.h"

static struct cursor {
    uint32_t*   screen;
    uint16_t    x, y; // Top left of the cursor
    uint16_t    vx, vy; // Top left of the cursor after applying scale (real / scale)
    uint16_t    width, height; // Size of the cursor, also size of a char
    uint16_t    vwidth, vheight; // Size of the cursor, after applying the scale (real / scale)
    uint32_t // Screen information
                s_pitch,
                s_width,
                s_height,
                s_vwidth, // Width of the screen after applying the scale (real / scale)
                s_vheight; // Height of the screen after applying the scale (real / scale)
    bool        hres; // Font high res toggle
} cursor;
static Bmft     *font;
static uint16_t scale;

// Put a pixel at a real position
void kputpixel(uint32_t color, uint16_t x, uint16_t y) {
    cursor.screen[y * (cursor.s_pitch) + x] = color;
}

// Put a pixel at a virtual position
void kvputpixel(uint32_t color, uint16_t x, uint16_t y) {
    for (uint16_t i = 0; i < scale; i++)
    for (uint16_t j = 0; j < scale; j++)
        kputpixel(color, x * scale + i, y * scale + j);
}

// Put the cursor at a real position
inline static void setcursor(uint16_t x, uint16_t y) {
    cursor.x = x;
    cursor.y = y;
    cursor.vx = x / scale;
    cursor.vy = y / scale;
}

// Put the cursor at a virtual position
inline static void setvcursor(uint16_t x, uint16_t y) {
    cursor.x = x * scale;
    cursor.y = y * scale;
    cursor.vx = x;
    cursor.vy = y;
}

// Set the x coordinate of the real cursor
inline static void setcursorx(uint16_t x) {
    cursor.x = x;
    cursor.vx = x / scale;
}

// Set the x coordinate of the virtual cursor
inline static void setvcursorx(uint16_t x) {
    cursor.x = x * scale;
    cursor.vx = x;
}

// Set the y coordinate of the real cursor
inline static void setcursory(uint16_t y) {
    cursor.y = y;
    cursor.vy = y / scale;
}

// Set the y coordinate of the virtual cursor
inline static void setvcursory(uint16_t y) {
    cursor.y = y * scale;
    cursor.vy = y;
}

int kterminit(BootInfo* bootInfo, uint16_t _scale, bool hres)
{
    font = (Bmft *)bootInfo->files->files[1].data;
    if (font->magicNumber != BMFT_MAGIC)
        return -1;
    if (font->version != BMFT_VERSION)
        return -2;

    scale = _scale;
    Framebuffer *fb = bootInfo->frameBuffer;
    cursor = (struct cursor){
        .screen = (uint32_t*)fb->addr,

        .x = 0,
        .y = 0,
        .vx = 0,
        .vy = 0,

        .width = 8 * (hres ? 1.5 : 1) * scale,
        .height = 12 * (hres ? 1.5 : 1) * scale,
        .vwidth = 8 * (hres ? 1.5 : 1),
        .vheight = 12 * (hres ? 1.5 : 1),

        .s_pitch = fb->pitch,
        .s_width = fb->width,
        .s_height = fb->height,
        .s_vwidth = fb->width / scale,
        .s_vheight = fb->height / scale,
        .hres = hres,
    };

    return 0;
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
}

void kclearline() {
    for (uint16_t y = cursor.vy; y < cursor.vy + cursor.vheight; y++)
    {
        for (uint16_t x = 0; x < cursor.s_vwidth; x++)
        {
            kvputpixel(0xFF000000, x, y);
        }
    }
}

void knewline()
{
    // if next block can't fit vertically, scroll
    if (cursor.y + 2U * cursor.height > cursor.s_height) {
        uint32_t *base = (uint32_t *)cursor.screen;

        for (uint64_t i = 0; i < cursor.s_height - cursor.height; i++)
        for (uint64_t j = 0; j < cursor.s_width; j++) {
            uint32_t *addr = base + cursor.s_width * i + j;
            *(addr) = *(addr + cursor.s_width * cursor.height);
        }
        kclearline();
    }
    else setcursory(cursor.y + cursor.height);
    setcursorx(0);
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
            setcursorx(0);
            break;
        case '\t':  // horz tab
            setcursorx(cursor.x + 4 * cursor.width);
            break;
        case '\b':  // backspace
            if (cursor.x == 0) break;
            setcursorx(cursor.x - cursor.width);
            for (uint32_t i = 0; i < cursor.width; i++)
            for (uint32_t j = 0; j < cursor.height; j++)
                kputpixel(0xFF000000, cursor.x + i, cursor.y + j);
            break;
        case '\a':  // bell
            kfillscreen(0xFFFF0000);
            break;
        default:
            setcursorx(cursor.x + cursor.width);
            break;
        }
    } else {
        if (cursor.x + cursor.width > cursor.s_width) knewline();

        for (uint32_t x = 0; x < cursor.vwidth; x++)
        for (uint32_t y = 0; y < cursor.vheight; y++) {
            bool pixel;
            switch (cursor.hres) {
                case 0: {
                    pixel = font->glpyhs[chr - PRINTABLE_ASCII_FIRST].px12[y] & (1<<x);
                    break;
                }
                case 1: {
                    pixel = font->glpyhs[chr - PRINTABLE_ASCII_FIRST].px18[y] & (1<<x);
                    break;
                }
            }
            kvputpixel(pixel ? 0xFFFFFFFF : 0, cursor.vx + x, cursor.vy + y);
        }
        setcursorx(cursor.x + cursor.width);
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

static inline void kprinthex(uint64_t n) {
    int8_t i = 0;
    if (n == 0) kputc('0');
    else {
        while (n > 0) {
            if (n%16 < 10) tmp[i++] = '0' + n%16;
            else tmp[i++] = 'A' + n%16 - 10;
            n /= 16;
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
                case 'x':
                    uint32_t x = va_arg(args, uint32_t);
                    kprinthex((uint64_t)x);
                    break;
                case 'X':
                    uint64_t lx = va_arg(args, uint64_t);
                    kprinthex((uint64_t)lx);
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
                        case 'x':
                            uint64_t lx = va_arg(args, uint64_t);
                            kprinthex(lx);
                            break;
                        default:
                            return;
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
