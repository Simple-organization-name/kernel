#include "boot.h"


_Noreturn void kernelMain(BootInfo* bootInfo) {
    Framebuffer *fbData = bootInfo->frameBuffer;
    volatile uint32_t *fb = (uint32_t *)fbData->addr;
    for (uint64_t i = 0; i < fbData->size / sizeof(uint32_t); i++) {
        fb[i] = ~fb[i];
    }

    while (1) __asm__("hlt");
}
