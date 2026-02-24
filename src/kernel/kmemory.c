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

PhysAddr _getPhysMemoryFromEFIMemMap(EfiMemMap *map, size_t nbpages) {
    uint64_t i = 0, start = 0;
    size_t found = 0;
    for (PhysAddr previousEnd = 0; i < map->count && found >= nbpages; i++) {
        MemoryDescriptor *desc = &map->map[i];
        if (!isValidMem(desc)) continue;

        if (previousEnd != desc->PhysicalStart) {
            previousEnd = desc->PhysicalStart;
            found = 0;
            start = i;
        }

        found += desc->NumberOfPages;
        if (found >= nbpages) {
            
        }
    }

    PRINT_ERR("Could not find enough contiguous memory\n");
    return -1UL;
}
