#include "boot/bootInfo.h"
#include "idt.h"
#include "kterm.h"
#include "asm.h"
#include "buddy.h"
#include "PCI.h"
#include "memmap.h"

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
    printBuddyTableInfo();

    // for (int i = 0; i < 10000000; i++) {
    //     buddyAlloc(BUDDY_2M);
    // }

    uint16_t idx[4] = {10, 0, 10, 0};
    if (findEmptyRangePageIdx(PTE_PT, idx, 600)) {
        kprintf("%u %u %u %u\n", idx[0], idx[1], idx[2], idx[3]);
    }
    
    kputs("\nHello from SOS kernel !\n");
    
    PCI_printAll();

    while (1) hlt();
}
