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

PhysAddr _getPhysMemoryFromEFIMemMap(EfiMemMap *physMemMap, size_t nbpages) {
    size_t found = 0;
    uint64_t iStart = 0, iEnd = 0;
    for (uint64_t i = 0; i < physMemMap->count && found < nbpages; i++) {
        MemoryDescriptor *desc = &physMemMap->map[i];

        if (!isValidMem(desc) ||
            physMemMap->map[i].PhysicalStart + physMemMap->map[i].NumberOfPages * 4096 != desc->PhysicalStart) { // Memory is not usable
            found = 0;
            iStart = iEnd = i;
        }

        found += desc->NumberOfPages;
        iEnd = i;
    }

    if (found >= nbpages) {
        uint64_t i = iEnd;
        for (; nbpages != 0; i--) {
            MemoryDescriptor *desc = &physMemMap->map[i];
            uint64_t min = desc->NumberOfPages < nbpages ? desc->NumberOfPages : nbpages;
            desc->NumberOfPages -= min;
            nbpages -= min;
        }
        return physMemMap->map[iStart].PhysicalStart + physMemMap->map[iStart].NumberOfPages * 4096;
    }

    PRINT_ERR("Could not find enough contiguous memory\n");
    return ADDR_MAX;
}
