#include "attribute.h"
#include "boot/bootInfo.h"
#include "idt.h"
#include "kterm.h"
#include "asm.h"
#include "buddy.h"
#include "PCI.h"
#include "memmap.h"
#include "kvmalloc.h"

_Noreturn void kmain(BootInfo* bootInfo)
{
    init_interrupts();

    if (kterminit(bootInfo, 1, 0)) CRIT_HLT();
    kfillscreen(0xFF000000);

    PhysAddr kernelPhysAddr = getMapping(0xFFFFFF7F80000000, NULL);
    PRINT_WARN("Kernel at 0x%X\n", kernelPhysAddr);
    PhysAddr fbPhysAddr = getMapping(0xFFFFFF7F40000000, NULL);
    PRINT_WARN("Framebuffer at 0x%X\n\n", fbPhysAddr);

    initBuddy(bootInfo->memMap);
    // printBuddyTableInfo();

    uint16_t idx[4] = {10, 0, 0, 0};
    __attribute_maybe_unused__ int *test = kvmalloc(idx, 50, 0);
    // if (test) {
    //     for (int i = 0; i < 1024*50; i++) {
    //         test[i] = i;
    //         // kprintf("%d ", test[i]);
    //     }
    //     kprintf("\ntest at: 0x%X\n", test);
    // } else PRINT_WARN("MSLQDKQSMLDK\n");

    kputs("\nHello from SOS kernel !\n");

    PCI_printAll();

    while (1) hlt();
}
