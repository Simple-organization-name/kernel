#include "boot.h"
#include "idt.h"
#include "kterm.h"
#include "asm.h"
#include "kalloc.h"

_Noreturn void _start(BootInfo* bootInfo)
{
    if (kterminit(bootInfo, 1, 1)){
        cli();
        while (1) hlt();
    }

    kfillscreen(0xFF000000);

    physMemoryMap = bootInfo->memMap;

    init_interrupts();
    initPhysMem();

    kputs("Hello from SOS kernel !\n");
    kputs("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890,?;.:/!*$&~\"#'{}()[]-|`_\\^@+=<>\n");

    physAddr kernelPhysAddr = getMapping(0xFFFFFF7F80000000);
    kprintf("%X\n", kernelPhysAddr);

    if (!unmapPage(0xFFFFFF7F80000000)) kputs("Failed to unmap kernel\n");
    kputs("Test done\n"); // Should not be printed as the kernel has been unmapped

    // if (!mapPage(validMemory[0], 0xFFFF643200000000)) kputs("Failed to map page\n");
    // uint64_t *ptr = (uint64_t *)0xFFFF643200000000;

    while (1) hlt();
}
