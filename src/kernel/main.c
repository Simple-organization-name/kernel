#include "boot.h"
#include "idt.h"
#include "kterm.h"

_Noreturn void _start(BootInfo* bootInfo)
{
    if (kterminit(bootInfo))
        while (1) __asm__(HLT);

    kfillscreen(0xFF000000);

    init_interrupts();

    kputc(' ');
    for (int i = 0; i < 45; i++)
    kputs("Hello from SOS kernel !\n");
    // kprintf("");

    while (1);
}
