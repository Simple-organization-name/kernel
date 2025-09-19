#include "boot.h"
#include "idt.c"
#include "visu.c"
#include <stdarg.h>

_Noreturn void _start(BootInfo* bootInfo, ...)
{
    init_visu(bootInfo, 12);
    fill_screen(0xFF000000);

    init_interrupts();


    puts("HELLO !");

    

    while (1);
}
