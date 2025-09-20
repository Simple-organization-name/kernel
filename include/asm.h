#ifndef __ASM_H__
#define __ASM_H__

#include <stdint.h>

inline static uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile(
        "inb %1, %0"
        : "=a"(value)
        : "d"(port)
        : "memory"
    );
    return value;
}

inline static void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile(
        "outb %1, %0"
        :
        : "d"(port), "a"(value)
        : "memory"
    );
}

inline static void hlt()
{
    __asm__ volatile ("hlt");
}

inline static void cli()
{
    __asm__ volatile ("cli");
}

inline static void sti()
{
    __asm__ volatile ("sti");
}

#endif