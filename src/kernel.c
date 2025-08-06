#include "boot.h"

// Force calling convention to ms_abi cause bootloader is built in PE32+ format
_Noreturn __attribute__((ms_abi))
void kernelMain(Framebuffer* fbData) {
    volatile uint32_t *fb = (uint32_t *)fbData->addr;
    for (uint64_t i = 0; i < fbData->size / sizeof(uint32_t); i++) {
        fb[i] = ~fb[i];
    }

    while (1) __asm__("hlt");
}
