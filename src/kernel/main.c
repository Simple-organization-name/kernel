#include "boot.h"
#include "idt.h"
#include "kterm.h"
#include "asm.h"
#include "kalloc.h"
#include "PCI.h"

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

    kputs("\nBefore alloc:\n");
    printMemBitmapLevel(0);
    char *test = (char *)kallocPage(MEM_4K);
    kprintf("Test using kallocPage at 0x%X\n", test);
    kputs("After alloc:\n");
    printMemBitmapLevel(0);
    uint8_t i = 0;
    for (char c = 'a'; c <= 'z'; c++) {
        test[i++] = c;
    }
    test[i] = 0;
    kputs(test);
    kfreePage(test);
    kputs("After free:\n");
    printMemBitmapLevel(0);

    kputs("Hello from SOS kernel !\n");
    
    // printAllPCI();

    // kputs("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890,?;.:/!*$&~\"#'{}()[]-|`_\\^@+=<>\n");

    while (1) hlt();
}
