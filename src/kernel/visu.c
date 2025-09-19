#include <stdint.h>
#include <stddef.h>

#include <boot.h>
#include "bmft.h"

struct cursor {
    uint32_t* screen;
    uint16_t x, y,
        size,
        s_pitch,
        s_height,
        s_width;
};


static struct cursor where;
static Bmft *font;

void init_visu(register BootInfo* bootInfo, register uint16_t size)
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

// void load_bmft(void* data)
// {
//     font = NULL;
// }

inline static void put_pixel(register uint32_t color, register uint16_t x, register uint16_t y)
{
    where.screen[y*where.s_pitch + x] = color;
}

void fill_screen(register uint32_t color)
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
    where.y += where.size;
    // if next block can't fit vertically, wrap around
    if (where.y + where.size > where.s_height)
        where.y = 0;
    // clear the line just in case junk is left over
    for (uint16_t y = where.y; y < where.y + where.size; y++)
    {
        for (uint16_t x = 0; x < where.s_width; x++)
        {
            put_pixel(0xFF000000, x, y);
        }
    }
}

void log_color(register uint32_t color)
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

void putc(uint8_t chr)
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
            where.x -= where.size;
            log_color(0xFF000000);
            where.x -= where.size;
            break;
        case '\a':  // bell
            fill_screen(0xFFFF0000);
            break;
        default:
            break;
        }
    } else {
        for (uint32_t x = 0; x < 8; x++)
            for (uint32_t y = 0; y < 12; y++)
                if (font->glpyhs[chr - PRINTABLE_ASCII_FIRST].px12[y] & (1<<x))
                    put_pixel(0xFFFFFFFF, where.x + x, where.y + y);
        where.x += 12;
    }
}

void puts(const char* str)
{
    while (*str) {
        putc(*(str++));
    }
}



