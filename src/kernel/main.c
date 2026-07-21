#include <attribute.h>
#include <boot/bootInfo.h>
#include <IDT.h>
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

    PhysAddr kernelPhysAddr = memmap_getMapping(0xFFFFFF7F80000000UL, NULL);
    PRINT_DEBUG("Kernel at %p\n", kernelPhysAddr);
    PhysAddr fbPhysAddr = memmap_getMapping(0xFFFFFF7F40000000UL, NULL);
    PRINT_DEBUG("Framebuffer at %p\n\n", fbPhysAddr);

    init_kdbg(&bootInfo->files->files[0]);

    printEFIMemMap(bootInfo->memMap);
    // CRIT_HLT();

    buddy_init(bootInfo->memMap);
    // buddy_printTable();

    printEFIMemMap(bootInfo->memMap);
    // memmap_printMapping();

    #define nb 3500
    PRINT_DEBUG("Test with %U pages (%UB)\n", nb, nb*(1<<12));
    __attribute_maybe_unused__ int *test = kvmalloc(nb, 0);
    PRINT_DEBUG("c\n");

    memmap_printMapping();

    if (test) {
        for (uint64_t i = 0; i < ((1<<12)*nb) / sizeof(int); i++) {
            if (i % (1<<10) == 0 && (VirtAddr)(test + i) > 0x8000BF4000)
                PRINT_DEBUG("va: %p, pa: %p, val: %x\n", test + i, memmap_getMapping((VirtAddr)(test + i), NULL), test[i]);
            test[i] = i;

            if (i % (1<<10) == 0 && (VirtAddr)(test + i) > 0x8000BF4000)
                PRINT_DEBUG("va: %p, pa: %p, val: %x\n", test + i, memmap_getMapping((VirtAddr)(test + i), NULL), test[i]);
            // kprintf("%d ", test[i]);
        }
        PRINT_DEBUG("Test at: %p\n", test);
    } else PRINT_WARN("Failed to get memory from kvmalloc\n");

    kputs("\nHello from SOS kernel !\n");

    PCI_printAll();

    while (1) hlt();
}
