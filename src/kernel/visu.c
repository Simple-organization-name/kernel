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

void init_cursor(register Framebuffer* restrict fb, register struct cursor* restrict cur, register uint16_t size)
{
    *cur = (struct cursor){
        .screen = (uint32_t*)fb->addr,
        .x = 0,
        .y = 0,
        .size = size,
        .s_pitch = fb->pitch,
        .s_height = fb->height,
        .s_width = fb->width
    };
}

inline void put_pixel(register struct cursor* cur, register uint32_t color, register uint16_t x, register uint16_t y)
{
    cur->screen[y*cur->s_pitch + x] = color;
}

void fill_screen(register struct cursor* cur, register uint32_t color)
{
    
    for (uint16_t i = 0; i < cur->s_height; i++)
    {
        for (uint16_t j = 0; j < cur->s_width; j++)
        {
            put_pixel(cur, color, j, i);
        }
    }
    cur->x = 0;
    cur->y = 0;
}

void new_line(register struct cursor* cur)
{
    cur->x = 0;
    cur->y += cur->size;
    // if next block can't fit vertically, wrap around
    if (cur->y + cur->size > cur->s_height)
        cur->y = 0;
    // clear the line just in case junk is left over
    for (uint16_t y = cur->y; y < cur->y + cur->size; y++)
    {
        for (uint16_t x = 0; x < cur->s_width; x++)
        {
            put_pixel(cur, 0xFF000000, x, y);
        }
    }
}

void log_color(register struct cursor* cur, register uint32_t color)
{
    if (cur->x + cur->size > cur->s_width)
    new_line(cur);
    
    for (uint16_t i = cur->y; i < cur->y + cur->size; i++)
    {
        for (uint16_t j = cur->x; j < cur->x + cur->size; j++)
        {
            put_pixel(cur, color, j, i);
        }
    }
    cur->x += cur->size;
}

