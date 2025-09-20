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
#define PML4() (PD_BASE)
#define PDPT(pml4_i) (PML4() | pml4_i << 30)
#define PD(pml4_i, pdpt_i) (PDPT(pml4_i) | pdpt_i << 21)
#define PT(pml4_i, pdpt_i, pd_i) (PD(pml4_i, pdpt_i) | pd_i << 12)

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

    uint32_t pml3_index = (virtual >> 30) & 0x1FF;
    entry = ((pte_t *)PDPT(pml4_index))[pml3_index];
    if (!entry.present) return 0;
    if (entry.pageSize) return entry.whole | PTE_ADDR;

    uint32_t pml2_index = (virtual >> 21) & 0x1FF;
    entry = ((pte_t *)PD(pml4_index, pml3_index))[pml2_index];
    if (!entry.present) return 0;
    if (entry.pageSize) return entry.whole | PTE_ADDR;

    uint32_t pml1_index = (virtual >> 12) & 0x1FF;
    entry = ((pte_t *)PT(pml4_index, pml3_index, pml2_index))[pml1_index];
    if (!entry.present) return 0;
    if (entry.pageSize) return entry.whole | PTE_ADDR;

    return 0;
}

