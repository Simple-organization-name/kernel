#include "boot.h"
#include "idt.h"
#include "kterm.h"
#include "asm.h"
#include "kalloc.h"

_Noreturn void _start(BootInfo* bootInfo)
{
    if (kterminit(bootInfo, 1, 0)) {
        cli();
        while (1) hlt();
    }

    kfillscreen(0xFF000000);

    physMemoryMap = bootInfo->memMap;

    init_interrupts();
    
    physAddr kernelPhysAddr = getMapping(0xFFFFFF7F80000000, NULL);
    kprintf("Kernel at 0x%X\n", kernelPhysAddr);
    physAddr fbPhysAddr = getMapping(0xFFFFFF0000000000, NULL);
    kprintf("Framebuffer at 0x%X\n", fbPhysAddr);
    
    initPhysMem();

    kputs("Hello from SOS kernel !\n");
    kputs("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890,?;.:/!*$&~\"#'{}()[]-|`_\\^@+=<>\n");
    

    while (1) hlt();
}
