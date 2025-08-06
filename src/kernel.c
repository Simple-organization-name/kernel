#include "boot.h"

typedef void BootInfo;

_Noreturn void kernelMain(BootInfo* bootInfo);

_Noreturn void _start(Framebuffer* frmbf)
{
    uint32_t *fb = (uint32_t *)frmbf->addr;
    for (uint64_t i = 0; i < frmbf->size / sizeof(uint32_t); i++) {
        fb[i] = ~fb[i];
    }

    while (1);
}

_Noreturn void kernelMain(BootInfo* bootInfo)
{

    while(1 || bootInfo);
}