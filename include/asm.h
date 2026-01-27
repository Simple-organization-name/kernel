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

inline static uint32_t inl(uint16_t port)
{
    uint32_t value;
    __asm__ volatile(
        "inl %1, %0"
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

inline static void outl(uint16_t port, uint32_t value)
{
    __asm__ volatile(
        "outl %1, %0"
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

#define CRIT_HLT() do {cli(); while(1) hlt();} while (0)

inline static void sti()
{
    __asm__ volatile ("sti");
}

inline static void invlpg(uint64_t addr)
{
    __asm__ volatile (
        "invlpg %0"
        :: "m"(addr)
        : "memory"
    );
}

inline static uint64_t bsr64(uint64_t n) {
    uint64_t ret;
    __asm__ volatile (
        "bsrq %1, %0"
        : "=r"(ret)
        : "rm"(n)
    );
    return ret;
}

inline static uint64_t lzcnt64(uint64_t n) {
    uint64_t ret;
    __asm__ volatile (
        "lzcntq %1, %0"
        : "=r"(ret)
        : "rm"(n)
    );
    return ret;
}

#endif
