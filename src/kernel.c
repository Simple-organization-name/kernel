#include "boot.h"

typedef void BootInfo;

_Noreturn void kernelMain(BootInfo* bootInfo);

uint32_t _start(Framebuffer* frmbf)
{
    (void)frmbf;
    uint32_t *fb = (uint32_t *)frmbf->addr;
    for (uint64_t i = 0; i < frmbf->size / sizeof(uint32_t); i++) {
        fb[i] = ~fb[i];
    }
    for (uint64_t volatile i = 0; i < 1000000000; i++)
    {
        /* code */
    }
    
    return 0xFF00FF00;

}

_Noreturn void kernelMain(BootInfo* bootInfo)
{

    while(1 || bootInfo);
}