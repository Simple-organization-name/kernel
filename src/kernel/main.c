#include "boot.h"
#include "idt.h"
#include "visu.h"

_Noreturn void _start(BootInfo* bootInfo, ...)
{
    init_visu(bootInfo, 12);
    fill_screen(0xFF000000);

    init_interrupts();


    kputs("HELLO !");
    kprintf("%s %d %u %c %D %U %ld %lu\n", "Hello", -42, (uint32_t)-1, 'a', (long) - 1, (uint64_t)-1, (long)INT64_MAX - 1, (uint64_t)-1);

    while (1);
}
