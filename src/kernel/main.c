#include "boot.h"
// #include "../src/kernel/idt.c"
#include "visu.c"

_Noreturn void _start(BootInfo* bootInfo)
{
    struct cursor where;
    init_cursor(bootInfo->frameBuffer, &where, 20);
    fill_screen(&where, 0xFF000000);

    while (1)
    for (int i = 0; i <= 0xFF; i++)
    {
        log_color(&where, i);
        for (volatile int i = 0; i < 1000000; i++);
        log_color(&where, i << 8);
        for (volatile int i = 0; i < 1000000; i++);
        log_color(&where, i << 16);
        for (volatile int i = 0; i < 1000000; i++);
    }
    

    while (1);
}
