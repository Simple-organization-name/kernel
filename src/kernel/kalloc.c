#include "memTables.h"

#include <kalloc.h>


volatile        pte_t       *pml4;
volatile        MemMap      *physMemoryMap;
static volatile MemoryRange validMemory[512] = {0};
static volatile uint64_t    validMemoryCount = 0;

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

bool mapPage(physAddr physical, virtAddr virtual) {
    (void)physical;
    (void)virtual;
    return false;
}

bool unmapPage(virtAddr virtual) {
    (void)virtual;
    return false;
}

physAddr getMapping(virtAddr virtual) {
    uint32_t pml4_index = (virtual >> 39) & 0x1FF;
    pte_t entry = ((pte_t *)PML4())[pml4_index];
    if (!entry.present) return 0;
    
    uint32_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = ((pte_t *)PDPT(pml4_index))[pdpt_index];
    if (!entry.present) return 0;
    if (entry.pageSize) return entry.whole & PTE_ADDR;
    
    uint32_t pd_index = (virtual >> 21) & 0x1FF;
    entry = ((pte_t *)PD(pml4_index, pdpt_index))[pd_index];
    if (!entry.present) return 0;
    if (entry.pageSize) return entry.whole & PTE_ADDR;
    
    uint32_t pt_index = (virtual >> 12) & 0x1FF;
    entry = ((pte_t *)PT(pml4_index, pdpt_index, pd_index))[pt_index];
    if (!entry.present) return 0;
    if (entry.pageSize) return entry.whole & PTE_ADDR;

    return 0;
}

