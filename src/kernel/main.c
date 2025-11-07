#include "boot.h"
#include "idt.h"
#include "kterm.h"
#include "asm.h"
#include "kalloc.h"

_Noreturn void kmain(BootInfo* bootInfo)
{
    init_interrupts();

    if (kterminit(bootInfo, 1, 0)) {
        cli();
        while (1) hlt();
    }
    kfillscreen(0xFF000000);

    PhysAddr kernelPhysAddr = getMapping(0xFFFFFF7F80000000, NULL);
    kprintf("Kernel at 0x%X\n", kernelPhysAddr);
    PhysAddr fbPhysAddr = getMapping(0xFFFFFF7F40000000, NULL);
    kprintf("Framebuffer at 0x%X\n\n", fbPhysAddr);

    initPhysMem(bootInfo->memMap);
    printMemBitmap();

    PhysAddr test[100];
    kprintf("Reserving (phys mem) 100 pages of size 4K\n");
    for (uint8_t i = 0; i < 100; i++) {
        test[i] = resPhysMemory(MEM_4K, 1);
    }
    kprintf("Bitmap before free:\n");
    printMemBitmap();
    for (uint8_t i = 0; i < 100; i++) {
        freePhysMemory(test[i], MEM_4K);
    }
    kprintf("Bitmap after free:\n");
    printMemBitmap();


    // kputs("Testing map function...\n");
    // PhysAddr test = resPhysMemory(MEM_4K, 1);
    // kprintf("Test phys: 0x%X\n", test);
    // char *ptr = NULL;
    // if (!mapPage((VirtAddr *)&ptr, test, PTE_PT, (uint64_t)PTE_RW))
    //     kprintf("failed to map test\n");
    // else kprintf("Test at 0x%lx (virt)\n", ptr);

    // for (uint64_t i = 0; i < (2<<11); i++) {
    //     // kprintf("%d ", i);
    //     ptr[i] = i;
    // }
    // for (uint64_t i = 0; i < (2<<11); i++)
    //     kprintf("%d ", ptr[i]);

    kputs("Hello from SOS kernel !\n");
    // kputs("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890,?;.:/!*$&~\"#'{}()[]-|`_\\^@+=<>\n");

    while (1) hlt();
}
