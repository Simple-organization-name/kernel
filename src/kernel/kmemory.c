#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "asm.h"
#include "attribute.h"
#include "kterm.h"
#include "kmemory.h"

__attribute_maybe_unused__
void *memset(void *dest, int val, size_t count) {
    for (size_t i = 0; i < count; i++)
        ((uint8_t *)dest)[i] = val;
    return dest;
}

uint8_t getValidMemRanges(EfiMemMap *physMemoryMap, MemoryRange (*validMemory)[]) {
    uint8_t validMemoryCount = 0;
    for (uint64_t i = 0; (i < physMemoryMap->count) && (validMemoryCount < UINT8_MAX); i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemoryMap->map + physMemoryMap->descSize * i);
        switch (desc->Type) {
            case EfiLoaderCode:
            case EfiLoaderData:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiConventionalMemory:
                if (validMemoryCount > 0 &&
                    (*validMemory)[validMemoryCount - 1].start +
                    (*validMemory)[validMemoryCount - 1].size ==
                    desc->PhysicalStart)
                {
                    (*validMemory)[validMemoryCount - 1].size += desc->NumberOfPages * 4096;
                }
                else
                {
                    (*validMemory)[validMemoryCount].start = desc->PhysicalStart;
                    (*validMemory)[validMemoryCount].size = desc->NumberOfPages * 4096;
                    validMemoryCount++;
                }
                break;
            default:
                break;
        }
    }

    return validMemoryCount;
}

uint64_t getTotalRAM(EfiMemMap *physMemMap) {
    uint64_t total = 0;
    for (uint64_t i = 0; i < physMemMap->count; i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemMap->map + physMemMap->descSize * i);
        total += desc->NumberOfPages * 4096;
    }
    return total;
}

__attribute_no_vectorize__
PhysAddr _getPhysMemoryFromMemRanges(MemoryRange (*validMemory)[], uint8_t *validCount, size_t size) {
    int16_t where = -1;
    uint64_t totalSize = 0;
    PhysAddr memoryChunk = 0;
    for (uint8_t i = 0; i < *validCount; i++) {
        memoryChunk = ALIGN((*validMemory)[i].start, size);
        totalSize = memoryChunk - (*validMemory)[i].start + size;
        if ((*validMemory)[i].size >= totalSize) {
            where = i;
            break;
        }
    }

    if (where == -1) {
        PRINT_ERR("Could not find enough contiguous memory\n");
        return -1UL;
    }

    // If there's memory available before the start of the memory mapping made by the alignment
    if (memoryChunk != (*validMemory)[where].start) {
        // Shift all MemoryRange
        for (uint8_t i = *validCount - 1; i >= where; i--) {
            (*validMemory)[i + 1].start = (*validMemory)[i].start;
            (*validMemory)[i + 1].size = (*validMemory)[i].size;
        }

        // Segment target in 2 parts
        // First part is the part before the needed chunk
        (*validMemory)[where].size = memoryChunk - (*validMemory)[where].start;
        where++;
        (*validCount)++; // increase validCount as we just created a new valid memory range
        // Second part (right below) is after the needed chunk
    }
    (*validMemory)[where].start = memoryChunk + size;
    (*validMemory)[where].size -= totalSize;

    // If the new size of the memory range is 0, remove it from the valid memory
    if ((*validMemory)[where].size <= 0) {
        for (uint8_t i = where; i < *validCount - 1; i++) {
            (*validMemory)[i].start = (*validMemory)[i + 1].start;
            (*validMemory)[i].size = (*validMemory)[i + 1].size;
        }
    }

    return memoryChunk;
}

void clearPageTable(PhysAddr addr) {
    ((PageEntry *)PT(510, 508, 511))[0].whole = MAKE_PAGE_ENTRY(addr, PTE_P | PTE_NX | PTE_RW);
    invlpg((uint64_t)VA(510, 508, 511, 0));
    memset(VA(510, 508, 511, 0), 0, sizeof(PageEntry));
    ((PageEntry *)PT(510, 508, 511))[0].whole = 0;
    invlpg((uint64_t)VA(510, 508, 511, 0));
}

int unmapPage(VirtAddr virtual) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    PageEntry *entry = ((PageEntry *)PML4()) + pml4_index;
    if (!entry->present) return 1;

    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = ((PageEntry *)PDPT(pml4_index)) + pdpt_index;
    if (!entry->present) return 1;
    if (entry->pageSize) {
        entry->whole = 0;
        invlpg(virtual);
        return 0;
    }

    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = ((PageEntry *)PD(pml4_index, pdpt_index)) + pd_index;
    if (!entry->present) return 1;
    if (entry->pageSize) {
        entry->whole = 0;
        invlpg(virtual);
        return 0;
    }

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = ((PageEntry *)PT(pml4_index, pdpt_index, pd_index)) + pt_index;
    if (!entry->present) return 1;
    entry->whole = 0;
    invlpg(virtual);
    return 0;
}

PhysAddr getMapping(VirtAddr virtual, uint8_t *pageLevel) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    PageEntry entry = ((PageEntry *)PML4())[pml4_index];
    if (!entry.present) return -1;

    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = ((PageEntry *)PDPT(pml4_index))[pdpt_index];
    if (!entry.present) return -1;
    if (entry.pageSize) {
        if (pageLevel) *pageLevel = PTE_PDP;
        return entry.whole & PTE_ADDR;
    }

    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = ((PageEntry *)PD(pml4_index, pdpt_index))[pd_index];
    if (!entry.present) return -1;
    if (entry.pageSize) {
        if (pageLevel) *pageLevel = PTE_PD;
        return entry.whole & PTE_ADDR;
    }

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = ((PageEntry *)PT(pml4_index, pdpt_index, pd_index))[pt_index];
    if (!entry.present) return -1;
    if (pageLevel) *pageLevel = PTE_PT;
    return entry.whole & PTE_ADDR; // pt is always 4KiB so it doesn't have the pageSize flag
}
