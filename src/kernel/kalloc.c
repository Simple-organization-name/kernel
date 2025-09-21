#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <asm.h>

#include "memTables.h"
#include "kalloc.h"

volatile MemMap     *physMemoryMap      = NULL;
static MemoryRange  validMemory[512]    = {0};
static uint64_t     validMemoryCount    = 0;

#include <kterm.h>

inline static void initMemoryBitmap() {
    for (size_t i = 0; i < sizeof(MemBitmap); i++) {
        ((MemBitmap *)memoryBitmap_va)->bitmap[i] = 0;
    }
}

void initPhysMem() {
    for (uint64_t i = 0; i < physMemoryMap->count; i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemoryMap->map + physMemoryMap->descSize * i);
        switch (desc->Type) {
            case EfiLoaderCode:
            case EfiLoaderData:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiConventionalMemory:
                kprintf("Memory type: %d | ", desc->Type);
                if (validMemoryCount > 0 &&
                    validMemory[validMemoryCount - 1].start +
                    validMemory[validMemoryCount - 1].size ==
                    desc->PhysicalStart)
                {
                    kputs("Adjacent memory | ");
                    validMemory[validMemoryCount - 1].size += desc->NumberOfPages * 4096;
                } 
                else
                {
                    validMemory[validMemoryCount++] = (MemoryRange){
                        .start = desc->PhysicalStart,
                        .size = desc->NumberOfPages * 4096,
                    };
                }
                kprintf("Memory start: %X, memory size: %X\n", validMemory[validMemoryCount - 1].start, validMemory[validMemoryCount -1].size);
                break;
            default:
                break;
        }
    }

    #define align(addr) (((uint64_t)(addr) + 0x1FFFFF) & ~0x1FFFFF)
    int16_t where = -1;
    uint64_t totalSize = 0;
    physAddr bitmapBase = 0;
    for (uint16_t i = 0; i < validMemoryCount; i++)
    {
        kprintf("Segment start at %X, of size %X\n", validMemory[i].start, validMemory[i].size);
        bitmapBase = align(validMemory[i].start);
        totalSize = bitmapBase - validMemory[i].start + sizeof(MemBitmap);
        if (validMemory[i].size >= totalSize) {
            where = i;
            break;
        }
    }
    #undef align

    if (where == -1) {
        kprintf("Could not find enough contiguous memory for memory bitmap\n");
        cli();
        while (1) hlt();
    }

    validMemory[where].start += totalSize;
    validMemory[where].size -= totalSize;
    kprintf("Bitmap base: 0x%X\n", bitmapBase);

    ((pte_t *)PDPT(510))[1].whole = ((uintptr_t)bitmapBase & PTE_ADDR) | PTE_P | PTE_RW | PTE_PS | PTE_NX;
    invlpg(memoryBitmap_va);

    physAddr pdpt = getMapping((virtAddr)PDPT(510), NULL);
    kprintf("PDPT at %U\n", pdpt);

    uint8_t bitmapPageLevel = 0;
    bitmapBase = getMapping(memoryBitmap_va, &bitmapPageLevel);
    if (bitmapBase == 0) {
        kputs("Failed to map memory bitmap\n");
        cli();
        while (1) hlt();
    }
    kprintf("MemoryBitmap at 0x%X end at 0x%X, paging level %u\n", bitmapBase, bitmapBase + sizeof(MemBitmap), bitmapPageLevel);

    initMemoryBitmap();
}

// physAddr reserveMemory(size_t size) {
    
// }

#define map(physical) entry->whole = (uint64_t)((uintptr_t)physical & PTE_ADDR) | PTE_P | PTE_RW
bool mapPage(physAddr physical, virtAddr virtual) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    pte_t *entry = ((pte_t *)PML4()) + pml4_index;
    if (!entry->present) return 0;

    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = ((pte_t *)PDPT(pml4_index)) + pdpt_index;
    if (!entry->present) return 0;
    if (entry->pageSize) return 0;

    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = ((pte_t *)PD(pml4_index, pdpt_index)) + pd_index;
    if (!entry->present) return 0;
    if (entry->pageSize) return 0;

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = ((pte_t *)PT(pml4_index, pdpt_index, pd_index)) + pt_index;
    if (entry->present) return 0;
    map(physical);
    return 1;
}

bool unmapPage(virtAddr virtual) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    pte_t *entry = ((pte_t *)PML4()) + pml4_index;
    if (!entry->present) return 0;
    
    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = ((pte_t *)PDPT(pml4_index)) + pdpt_index;
    if (!entry->present) return 0;
    if (entry->pageSize) {
        entry->whole = 0;
        invlpg(virtual);
        return 1;
    }
    
    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = ((pte_t *)PD(pml4_index, pdpt_index)) + pd_index;
    if (!entry->present) return 0;
    if (entry->pageSize) {
        entry->whole = 0;
        invlpg(virtual);
        return 1;
    }

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = ((pte_t *)PT(pml4_index, pdpt_index, pd_index)) + pt_index;
    if (!entry->present) return 0;
    entry->whole = 0;
    invlpg(virtual);
    return 1;
}

physAddr getMapping(virtAddr virtual, uint8_t *pageLevel) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    pte_t entry = ((pte_t *)PML4())[pml4_index];
    if (!entry.present) return 0;

    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = ((pte_t *)PDPT(pml4_index))[pdpt_index];
    if (!entry.present) return 0;
    if (entry.pageSize) {
        if (pageLevel) *pageLevel = 3;
        return entry.whole & PTE_ADDR;
    }

    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = ((pte_t *)PD(pml4_index, pdpt_index))[pd_index];
    if (!entry.present) return 0;
    if (entry.pageSize) {
        if (pageLevel) *pageLevel = 2;
        return entry.whole & PTE_ADDR;
    }

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = ((pte_t *)PT(pml4_index, pdpt_index, pd_index))[pt_index];
    if (!entry.present) return 0;
    if (pageLevel) *pageLevel = 1;
    return entry.whole & PTE_ADDR; // pt is always 4KiB so it doesn't have the pageSize flag
}
