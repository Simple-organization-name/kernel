#include "memTables.h"
#include "kalloc.h"

volatile MemMap         *physMemoryMap     = NULL;
volatile MemoryRange    validMemory[512]   = {0};
volatile uint64_t       validMemoryCount   = 0;

void initPhysMem() {
    for (uint64_t i = 0; i < physMemoryMap->count; i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemoryMap->map + physMemoryMap->descSize * i);
        switch (desc->Type) {
            case EfiLoaderCode:
            case EfiLoaderData:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiConventionalMemory:
                validMemory[validMemoryCount++] = (MemoryRange){
                    .start = desc->PhysicalStart,
                    .size = desc->NumberOfPages * 4096,
                };
                break;
            default:
                break;
        }
    }
}

#define map(physical) entry->whole = (uint64_t)((uintptr_t)physical | PTE_ADDR) | PTE_P | PTE_RW
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

#include "kterm.h"
bool unmapPage(virtAddr virtual) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    pte_t *entry = ((pte_t *)PML4()) + pml4_index;
    if (!entry->present) return 0;
    
    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = ((pte_t *)PDPT(pml4_index)) + pdpt_index;
    if (!entry->present) return 0;
    if (entry->pageSize) {
        entry->whole = 0;
        return 1;
    }
    
    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = ((pte_t *)PD(pml4_index, pdpt_index)) + pd_index;
    if (!entry->present) return 0;
    if (entry->pageSize) {
        entry->whole = 0;
        return 1;
    }

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = ((pte_t *)PT(pml4_index, pdpt_index, pd_index)) + pt_index;
    if (!entry->present) return 0;
    entry->whole = 0;
    return 1;
}

physAddr getMapping(virtAddr virtual) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    pte_t entry = ((pte_t *)PML4())[pml4_index];
    if (!entry.present) return 0;

    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = ((pte_t *)PDPT(pml4_index))[pdpt_index];
    if (!entry.present) return 0;
    if (entry.pageSize) return entry.whole & PTE_ADDR;

    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = ((pte_t *)PD(pml4_index, pdpt_index))[pd_index];
    if (!entry.present) return 0;
    if (entry.pageSize) return entry.whole & PTE_ADDR;

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = ((pte_t *)PT(pml4_index, pdpt_index, pd_index))[pt_index];
    if (!entry.present) return 0;
    return entry.whole & PTE_ADDR; // pt is always 4KiB so it doesn't have the pageSize flag
}

