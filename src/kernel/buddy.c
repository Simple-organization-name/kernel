#include "buddy.h"
#include "asm.h"
#include "kterm.h"
#include "attribute.h"
#include "kmemory.h"

inline static uint8_t getValidMemRanges(EfiMemMap *physMemoryMap, MemoryRange *validMemory) {
    uint8_t validMemoryCount = 0;

    for (uint8_t i = 0; i < physMemoryMap->count; i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemoryMap->map + physMemoryMap->descSize * i);
        switch (desc->Type) {
            case EfiLoaderCode:
            case EfiLoaderData:
            case EfiBootServicesCode:
            case EfiBootServicesData:
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

__attribute_no_vectorize__
void initBuddy(EfiMemMap *physMemMap) {
    MemoryRange validMemory[256];
    uint8_t validCount = getValidMemRanges(physMemMap, validMemory);
    // for (int i = 0; i < validCount; i++) {
        MemoryRange range = validMemory[0];
        kprintf("%X: %U, %U\n", range.start, range.size, bsr64(range.size));
    // }
    (void)validCount;
}

