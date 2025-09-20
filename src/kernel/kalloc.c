#include "memTables.h"
#include "kalloc.h"

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

#define PD_BASE 0xFFFFFF0000000000
#define PAGE_TABLE(pml4_i, pdpt_i, pd_i, pt_i) ((pte_t *)((PD_BASE) | (pml4_i << 30) | (pdpt_i << 21) | (pd_i << 12) | (pt_i)))

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
    uint32_t pml3_index = (virtual >> 30) & 0x1FF;
    uint32_t pml2_index = (virtual >> 21) & 0x1FF;
    uint32_t pml1_index = (virtual >> 12) & 0x1FF;

    if (!PAGE_TABLE(pml4_index, 0, 0, 0)->present) return 0;
    if (!PAGE_TABLE(pml4_index, pml3_index, 0, 0)->present) return 0;
    if (!PAGE_TABLE(pml4_index, pml3_index, pml2_index, 0)->present) return 0;
    if (!PAGE_TABLE(pml4_index, pml3_index, pml2_index, pml1_index)->present) return 0;

    return PAGE_TABLE(pml4_index, pml3_index, pml2_index, pml1_index)->whole | PTE_ADDR;
}

