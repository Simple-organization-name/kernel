#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <asm.h>
#include <attribute.h>
#include <kterm.h>
#include <kmemory.h>

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiMaxMemoryType
} PhysicalMemoryType;

__attribute_maybe_unused__
void *memset(void *dest, int val, size_t count) {
    for (size_t i = 0; i < count; i++)
        ((char *)dest)[i] = val;
    return dest;
}

__attribute_maybe_unused__
void *memcpy(void * restrict dest, const void * restrict src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((char *)dest)[i] = ((char *)src)[i];
    }
    return dest;
}

int isValidMem(MemoryDescriptor *desc) {
    switch (desc->Type) {
        case EfiLoaderCode:
        case EfiLoaderData:
        case EfiBootServicesCode:
        case EfiBootServicesData:
        case EfiRuntimeServicesCode:
        case EfiRuntimeServicesData:
        case EfiConventionalMemory:
            return 1;
        default:
            return 0;
    }
}

uint64_t getTotalRAM(EfiMemMap *physMemMap) {
    uint64_t total = 0;
    for (uint64_t i = 0; i < physMemMap->count; i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemMap->map + physMemMap->descSize * i);
        total += desc->NumberOfPages * PAGE_SIZE;
    }
    return total;
}

void printEFIMemMap(EfiMemMap *physMemMap) {
    kputs("----==== EfiMemMap ====----\n");
    for (uint64_t i = 0; i < physMemMap->count; i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemMap->map + physMemMap->descSize * i);
        kprintf("start: %p, end: %p, nb of pages: %U, type: %u\n", desc->PhysicalStart, desc->PhysicalStart + desc->NumberOfPages * PAGE_SIZE, desc->NumberOfPages, desc->Type);
    }
    kputs("----===================----\n\n");
}

// Side effects: Modifies the EfiMemMap (removes some pages)
PhysAddr _getPagesFromEFIMemMap(EfiMemMap *physMemMap, size_t nbpages) {
    if (nbpages == 0) return ADDR_MAX;

    size_t found = 0;
    uint64_t iStart = 0, iEnd = 0;
    for (uint64_t i = 0; i < physMemMap->count && found < nbpages; i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemMap->map + physMemMap->descSize * i);
        MemoryDescriptor *end = (MemoryDescriptor *)((char *)physMemMap->map + physMemMap->descSize * iEnd);
        if (
            !isValidMem(desc) ||
            desc->NumberOfPages == 0 ||
            (i != iStart && end->PhysicalStart + end->NumberOfPages * PAGE_SIZE != desc->PhysicalStart)
        ) { // Memory is not usable
            // kprintf("end of range: %p, current start : %p\n", end->PhysicalStart + end->NumberOfPages * PAGE_SIZE, desc->PhysicalStart);
            found = 0;
            iStart = iEnd = i + 1;
            continue;
        }

        found += desc->NumberOfPages;
        iEnd = i;
    }

    if (found >= nbpages) {
        // Consume pages from the end of the range backwards to optimize (one write for backward vs two writes for forward)
        size_t remaining = nbpages;
        uint64_t i = iEnd;
        while (remaining != 0) {
            MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemMap->map + physMemMap->descSize * i);
            uint64_t min = desc->NumberOfPages < remaining ? desc->NumberOfPages : remaining;
            desc->NumberOfPages -= min;
            remaining -= min;
            if (remaining == 0) break;
            i--;
        }
        // The start address is at the end of the first partially-consumed descriptor
        MemoryDescriptor *start = (MemoryDescriptor *)((char *)physMemMap->map + physMemMap->descSize * i);
        PRINT_DEBUG("Reserved %U pages at %p\n", nbpages, start->PhysicalStart + start->NumberOfPages * PAGE_SIZE);
        return start->PhysicalStart + start->NumberOfPages * PAGE_SIZE;
    }

    PRINT_ERR("Could not find enough contiguous memory\n");
    return ADDR_MAX;
}
