#include "boot.h"
#include "idt.h"
#include "visu.h"

_Noreturn void _start(BootInfo* bootInfo, ...)
{
    init_visu(bootInfo, 12);
    fill_screen(0xFF000000);

    init_interrupts();


    kputs("HELLO !");
    kprintf("%s %d %u %c %D %U %ld %lu", "Hello", -42, (uint32_t)-1, 'a', INT64_MAX, (uint64_t)-1, INT64_MAX, (uint64_t)-1);
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
