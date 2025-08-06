#include "boot.h"

typedef void BootInfo;

_Noreturn MSABI void kernelMain(Framebuffer* frmbf)
{
    volatile uint32_t *fb = (uint32_t *)frmbf->addr;
    for (uint64_t i = 0; i < frmbf->size / sizeof(uint32_t); i++) {
        fb[i] = ~fb[i];
    }

    while (1) __asm__("hlt");
}
