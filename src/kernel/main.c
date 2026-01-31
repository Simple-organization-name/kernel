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

    // PhysAddr kernelPhysAddr = getMapping(0xFFFFFF7F80000000, NULL);
    // PRINT_WARN("Kernel at 0x%X\n", kernelPhysAddr);
    // PhysAddr fbPhysAddr = getMapping(0xFFFFFF7F40000000, NULL);
    // PRINT_WARN("Framebuffer at 0x%X\n\n", fbPhysAddr);

    initBuddy(bootInfo->memMap);
    printBuddyTableInfo();

    uint16_t idx[4] = {0};
    if (!findEmptySlotPageIdx(PTE_PT, idx)) {
        PRINT_WARN("Failed to find an empty space\n");
    } else {
        kprintf("%u %u %u %u -> %X\n", idx[0], idx[1], idx[2], idx[3], VA(idx[0], idx[1], idx[2], idx[3]));
        PhysAddr phys = buddyAlloc(0);
        
        mapPage(idx, PTE_PT, phys, PTE_RW | PTE_NX);
        char *test = VA_ARRAY(idx);
        kprintf("test phys: 0x%X, virt 0x%X\n", phys, test);
        for (uint8_t i = 0; i < UINT8_MAX; i++) {
            test[i] = i;
        }

        kprintf("test array:\n");
        for (uint8_t i = 0; i < UINT8_MAX; i++) {
            kprintf("%d ", test[i]);
        }
    }
    
    kputs("\nHello from SOS kernel !\n");

    printAllPCI();

    while (1) hlt();
}
