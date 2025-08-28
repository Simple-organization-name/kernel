#include "boot.h"


_Noreturn void _start(BootInfo* bootInfo) {
    Framebuffer *fbData = bootInfo->frameBuffer;
    volatile uint32_t *fb = (uint32_t *)fbData->addr;
    for (uint32_t i = 0; i < fbData->height; i++)
    {
        for (uint32_t j = 0; j < fbData->width; j++)
        {
            register uint64_t where = i*fbData->pitch + j;
            fb[where] = 0xFF000000 | (((0xFF * i / fbData->height) & 0xFF) << 16) | ((0xFF * j / fbData->width ) & 0xFF);
        }
        
    }
    

    while (1);
}
