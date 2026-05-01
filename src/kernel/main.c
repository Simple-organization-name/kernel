#include <attribute.h>
#include <boot/bootInfo.h>
#include <idt.h>
#include <kterm.h>
#include <asm.h>
#include <buddy.h>
#include <PCI.h>
#include <memmap.h>
#include <kvmalloc.h>
#include <kdbg.h>

_Noreturn void kmain(BootInfo* bootInfo)
{
    init_interrupts();

    if (kterminit(bootInfo, 1, 0)) CRIT_HLT();
    kfillscreen(0xFF000000);

    PhysAddr kernelPhysAddr = getMapping(0xFFFFFF7F80000000UL, NULL);
    PRINT_WARN("Kernel at 0x%X\n", kernelPhysAddr);
    PhysAddr fbPhysAddr = getMapping(0xFFFFFF7F40000000UL, NULL);
    PRINT_WARN("Framebuffer at 0x%X\n\n", fbPhysAddr);

    init_kdbg(&bootInfo->files->files[0]);

    // printEFIMemMap(bootInfo->memMap);
    // CRIT_HLT();

    initBuddy(bootInfo->memMap);
    printBuddyTableInfo();

    #define nb 100
    __attribute_maybe_unused__ int *test = kvmalloc(nb, 0);
    if (test) {
        for (int i = 0; i < 1024*nb; i++) {
            test[i] = i;
            // kprintf("%d ", test[i]);
        }
        kprintf("\ntest at: 0x%X\n", test);
    } else PRINT_WARN("MSLQDKQSMLDK\n");

    kputs("\nHello from SOS kernel !\n");

    PCI_printAll();

    while (1) hlt();
}
