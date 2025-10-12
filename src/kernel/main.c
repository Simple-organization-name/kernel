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

    init_interrupts();

    PhysAddr kernelPhysAddr = getMapping(0xFFFFFF7F80000000, NULL);
    kprintf("Kernel at 0x%X\n", kernelPhysAddr);
    PhysAddr fbPhysAddr = getMapping(0xFFFFFF0000000000, NULL);
    kprintf("Framebuffer at 0x%X\n", fbPhysAddr);

    initPhysMem(bootInfo->memMap);
    // printMemBitmap();

    // PhysAddr test = resPhysMemory(MEM_4K, 1);
    // char *ptr = NULL;
    // if (!mapPage((VirtAddr *)&ptr, test, PTE_PT, (uint64_t)PTE_RW))
    //     kprintf("failed to map test\n");
    // for (uint64_t i = 0; i < (2<<11); i++) {
    //     kprintf("%d ", i);
    //     ptr[i] = i;
    // }
    // for (uint64_t i = 0; i < (2<<11); i++)
    //     kprintf("%d ", ptr[i]);

    kputs("Hello from SOS kernel !\n");
    // kputs("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890,?;.:/!*$&~\"#'{}()[]-|`_\\^@+=<>\n");

    while (1) hlt();
}
