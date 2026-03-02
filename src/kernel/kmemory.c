#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "asm.h"
#include "attribute.h"
#include "kterm.h"
#include "kmemory.h"

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
        ((uint8_t *)dest)[i] = val;
    return dest;
}

__attribute_maybe_unused__
void *memcpy(void * restrict dest, const void * restrict src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((char *)dest)[i] = ((char *)src)[i];
    }
    return dest;
}

static int isValidMem(MemoryDescriptor *desc) {
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
        total += desc->NumberOfPages * 4096;
    }
    return total;
}

void printEFIMemMap(EfiMemMap *physMemMap) {
    kputs("----==== EfiMemMap ====----\n");
    for (uint64_t i = 0; i < physMemMap->count; i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemMap->map + physMemMap->descSize * i);
        kprintf("start: 0x%X, end: 0x%X, nb of pages: %U\n", desc->PhysicalStart, desc->PhysicalStart + desc->NumberOfPages * 4096, desc->NumberOfPages);
    }
    kputs("----===================----\n\n");
}

// Side effects: Modifies the EfiMemMap (removes some pages)
PhysAddr _getPhysMemoryFromEFIMemMap(EfiMemMap *physMemMap, size_t nbpages) {
    size_t found = 0;
    uint64_t iEnd = 0;
    for (uint64_t i = 0; i < physMemMap->count && found < nbpages; i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemMap->map + physMemMap->descSize * i);
        MemoryDescriptor *end = (MemoryDescriptor *)((char *)physMemMap->map + physMemMap->descSize * iEnd);
        if (
            !isValidMem(desc) || 
            desc->NumberOfPages == 0 ||
            end->PhysicalStart + end->NumberOfPages * 4096 != desc->PhysicalStart
        ) { // Memory is not usable
            // kprintf("end of range: 0x%X, current start : 0x%X\n", end->PhysicalStart + end->NumberOfPages * 4096, desc->PhysicalStart);
            found = 0;
            iEnd = i;
        }

        found += desc->NumberOfPages;
        iEnd = i;
    }

    if (found >= nbpages) {
        uint64_t i = iEnd;
        for (; nbpages != 0; i--) {
            MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemMap->map + physMemMap->descSize * i);
            uint64_t min = desc->NumberOfPages < nbpages ? desc->NumberOfPages : nbpages;
            desc->NumberOfPages -= min;
            nbpages -= min;
        }
        // kprintf("istart: %U, i: %U\n", iStart, i);
        MemoryDescriptor *start = (MemoryDescriptor *)((char *)physMemMap->map + physMemMap->descSize * i);
        return start->PhysicalStart + start->NumberOfPages * 4096;
    }

    PRINT_ERR("Could not find enough contiguous memory\n");
    return ADDR_MAX;
}
