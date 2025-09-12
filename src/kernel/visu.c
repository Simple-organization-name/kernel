#include <stdint.h>
#include <stdalign.h>

#include <boot.h>

struct cursor {
    uint32_t* screen;
    uint16_t x, y,
        size,
        s_pitch,
        s_height,
        s_width;
};

static struct cursor where;

void init_visu(register Framebuffer* fb, register uint16_t size)
{
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

