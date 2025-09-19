#include "boot.h"
#include "idt.c"
#include "visu.c"

_Noreturn void _start(BootInfo* bootInfo)
{
    init_visu(bootInfo, 12);
    fill_screen(0xFF000000);

    init_interrupts();


    puts("HELLO !");
    // while (1)
    // for (int i = 0; i <= 0xFF; i++)
    // {
    //     log_color(i);
    //     for (volatile int i = 0; i < 3000000; i++);
    //     log_color(i << 8);
    //     for (volatile int i = 0; i < 3000000; i++);
    //     log_color(i << 16);
    //     for (volatile int i = 0; i < 3000000; i++);
    // }
    

    while (1);
}
