#include "boot.h"
#include "idt.h"
#include "kterm.h"

_Noreturn void _start(BootInfo* bootInfo, ...)
{
    kterminit(bootInfo);
    kfillscreen(0xFF000000);

    init_interrupts();


    kputs("Hello from SOS kernel !\n");    

    while (1);
}
