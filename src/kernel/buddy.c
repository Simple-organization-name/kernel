#include "buddy.h"
#include "asm.h"
#include "kterm.h"
#include "attribute.h"
#include "kmemory.h"

static BuddyTable buddyTable;

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

void buddyFree(BuddyTable *table, PhysAddr addr, int level) {
    

}

PhysAddr buddyAlloc(BuddyTable *table, int level) {
    // also do stuff
    BuddyLevel* levels = table->levels;
    // if (levels[level].mem) {
    //     Buddy *tmp = levels[level].mem;
    //     levels[level].mem = levels[level].mem->next;
    //     tmp->next = table->usable_buddies;
    //     table->usable_buddies = tmp;
    //     return tmp->start;
    // } else  {

    // }
    int cur_level;
    for (cur_level = level; !levels[level].mem; cur_level++); // find nearest usable buddy iykyk
    while (cur_level != level) {
        
        level--;
    }

    
    return 0;
}

__attribute_no_vectorize__
void initBuddy(EfiMemMap *physMemMap) {
    MemoryRange validMemory[256];
    uint8_t validCount = getValidMemRanges(physMemMap, validMemory);

    #define align(addr) (((uint64_t)(addr) + 0x1FFFFF) & ~0x1FFFFF)
    int16_t where = -1;
    uint64_t totalSize = 0;
    PhysAddr memoryChunk = 0;
    for (uint8_t i = 0; i < validCount; i++) {
        memoryChunk = align(validMemory[i].start);
        totalSize = memoryChunk - validMemory[i].start + (2 << 20);
        if (validMemory[i].size >= totalSize) {
            where = i;
            break;
        }
    }
    #undef align

    if (where == -1) {
        kprintf("Could not find enough contiguous memory for memory bitmap\n");
        CRIT_HLT();
    }

    // If there's memory available before the start of the memory mapping made by the alignment
    if (memoryChunk != validMemory[where].start) {
        // Shift all MemoryRange
        for (uint8_t i = validCount - 1; i >= where; i--) {
            MemoryRange tmp = validMemory[i];
            validMemory[i + 1].size = tmp.size;
            validMemory[i + 1].start = tmp.start;
        }

        // Segment target in 2 parts
        // First part is the part before the MemBitmap
        validMemory[where].size = memoryChunk - validMemory[where].start;
        where++;
        // Second part (right below) is after the MemBitmap
    }
    validMemory[where].start = memoryChunk + (2 << 20);
    validMemory[where].size -= totalSize;

    ((PageEntry *)PD(510, 509))[511].whole = (uint64_t)(((uintptr_t)memoryChunk & PTE_ADDR) | PTE_P | PTE_RW | PTE_PS | PTE_NX);
    Buddy *buf = (Buddy *)VIRT_ADDR(510, 509, 511, 0);
    invlpg((uint64_t)buf);

    // Linked list init for the usable buddies :)
    uint64_t count = (1 << 20) / sizeof(Buddy);
    if (count == 0) CRIT_HLT();
    for (uint64_t i = 0; i < (count - 1); i++) {
        buf[i].next = &buf[i+1];
    }
    buf[count - 1].next = NULL;
    buddyTable.usable = buf;
}

