#include "boot.h"
#include "idt.h"
#include "kterm.h"
#include "asm.h"

_Noreturn void _start(BootInfo* bootInfo)
{
    if (kterminit(bootInfo, 2))
        while (1) { cli(); hlt(); }

    kfillscreen(0xFF000000);

    init_interrupts();

    kputs("Hello from SOS kernel !\n");
    // kprintf("");

    while (1);
}
