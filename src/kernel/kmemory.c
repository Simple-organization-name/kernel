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

uint8_t getValidMemRanges(EfiMemMap *physMemoryMap, MemoryRange *validMemory) {
    uint8_t validMemoryCount = 0;
    for (uint64_t i = 0; (i < physMemoryMap->count) && (validMemoryCount < UINT8_MAX); i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemoryMap->map + physMemoryMap->descSize * i);
        switch (desc->Type) {
            case EfiLoaderCode:
            case EfiLoaderData:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiRuntimeServicesCode:
            case EfiRuntimeServicesData:
            case EfiConventionalMemory:
                if (validMemoryCount > 0 &&
                    validMemory[validMemoryCount - 1].start +
                    validMemory[validMemoryCount - 1].size ==
                    desc->PhysicalStart)
                {
                    validMemory[validMemoryCount - 1].size += desc->NumberOfPages * 4096;
                }
                else
                {
                    validMemory[validMemoryCount].start = desc->PhysicalStart;
                    validMemory[validMemoryCount].size = desc->NumberOfPages * 4096;
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
PhysAddr _getPhysMemoryFromMemRanges(MemoryRange *validMemory, uint8_t *validCount, size_t size) {
    int16_t where = -1;
    uint64_t totalSize = 0;
    PhysAddr memoryChunk = 0;
    for (uint8_t i = 0; i < *validCount; i++) {
        memoryChunk = ALIGN(validMemory[i].start, size);
        totalSize = memoryChunk - validMemory[i].start + size;
        if (validMemory[i].size >= totalSize) {
            where = i;
            break;
        }
    }

    if (where == -1) {
        PRINT_ERR("Could not find enough contiguous memory\n");
        return -1UL;
    }

    // If there's memory available before the start of the memory mapping made by the alignment
    if (memoryChunk != validMemory[where].start) {
        // Shift all MemoryRange
        for (uint8_t i = *validCount - 1; i >= where; i--) {
            validMemory[i + 1].start = validMemory[i].start;
            validMemory[i + 1].size = validMemory[i].size;
        }

        // Segment target in 2 parts
        // First part is the part before the needed chunk
        validMemory[where].size = memoryChunk - validMemory[where].start;
        where++;
        (*validCount)++; // increase validCount as we just created a new valid memory range
        // Second part (right below) is after the needed chunk
    }
    validMemory[where].start = memoryChunk + size;
    validMemory[where].size -= totalSize;

    // If the new size of the memory range is 0, remove it from the valid memory
    if (validMemory[where].size <= 0) {
        for (uint8_t i = where; i < *validCount - 1; i++) {
            validMemory[i].start = validMemory[i + 1].start;
            validMemory[i].size = validMemory[i + 1].size;
        }
    }

    return memoryChunk;
}
