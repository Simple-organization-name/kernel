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
    
    pml4 = bootInfo->pml4;
    physMemoryMap = bootInfo->memMap;
    
    init_interrupts();
    initPhysMem();
    
    kputs("Hello from SOS kernel !\n");
    kputs("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890,?;.:/!*$&~\"#'{}()[]-|`_\\^@+=<>");

    physAddr kernelPhysAddr = getMapping(0xFFFFFFFF80000000);
    kprintf("%U\n", kernelPhysAddr);

    cli();
    while (1) hlt();
}
