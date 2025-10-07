#include "boot.h"
#include "idt.h"
#include "kterm.h"
#include "asm.h"
#include "kalloc.h"

_Noreturn void kmain(BootInfo* bootInfo)
{
    if (kterminit(bootInfo, 1, 0)) {
        cli();
        while (1) hlt();
    }

    kfillscreen(0xFF000000);

    physMemoryMap = bootInfo->memMap;

    init_interrupts();

    PhysAddr kernelPhysAddr = getMapping(0xFFFFFF7F80000000, NULL);
    kprintf("Kernel at 0x%X\n", kernelPhysAddr);
    PhysAddr fbPhysAddr = getMapping(0xFFFFFF0000000000, NULL);
    kprintf("Framebuffer at 0x%X\n", fbPhysAddr);

    initPhysMem();
    printMemBitmap();

    for (uint8_t i = 0; i < 10; i++) {
        PhysAddr test = resPhysMemory(MEM_4K, 2);
        if (test == -1UL)
            kputs("Failed to reserve memory");
        else
            kprintf("Memory reserved at %X\n", test);
    }
    knewline();
    
    PhysAddr a = resPhysMemory(MEM_2M, 1);
    kprintf("big at %X\n", a);

    kputs("Hello from SOS kernel !\n");
    // kputs("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890,?;.:/!*$&~\"#'{}()[]-|`_\\^@+=<>\n");

    while (1) hlt();
}
