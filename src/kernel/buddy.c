#include "buddy.h"
#include "asm.h"
#include "kterm.h"
#include "attribute.h"
#include "kmemory.h"

static BuddyTable buddyTable;

inline static uint8_t getValidMemRanges(EfiMemMap *physMemoryMap, MemoryRange (*validMemory)[256]) {
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

PhysAddr buddyTransfer(Buddy **src, Buddy **dest) {
    if (!src || !*src) return 1;
    Buddy *tmp = *src;
    *src = (*src)->next;
    tmp->next = *dest;
    *dest = tmp;
    return 0;
}

// THOU SHALL NOT FAIL
Buddy *grabUsableBuddy(BuddyTable *src) {
    if (!src->usable) {
        CRIT_HLT();
        // PhysAddr tmp[BUDDY_MAX_ORDER];
        // unsigned level = 0;
        // while (level < BUDDY_MAX_ORDER && src->levels[level].list) level++;
    }
    Buddy *out = src->usable;
    src->usable = src->usable->next;
    out->next = NULL;
    return out;
}

PhysAddr buddyAlloc(BuddyTable *table, uint8_t level) {
    BuddyLevel* levels = table->levels;

    uint8_t curLevel;
    for (curLevel = level; curLevel < BUDDY_MAX_ORDER && !levels[curLevel].list; curLevel++); // find nearest usable buddy iykyk
    if (curLevel == BUDDY_MAX_ORDER) {
        PRINT_ERR("[FATAL] OOM :p");
        CRIT_HLT();
    }
    while (curLevel != level) {
        buddyTransfer(&levels[curLevel].list, &levels[curLevel-1].list);
        Buddy *tmp = grabUsableBuddy(table);
        tmp->next = levels[curLevel - 1].list;
        levels[curLevel - 1].list = tmp;
    }
    // we are sure that we've got memory and grabUsableBuddy handles 
    // ooms on its own so we can assume levels[level].list != NULL
    return buddyTransfer(&levels[level].list, &table->usable);
}

void buddyFree(BuddyTable *table, PhysAddr addr, uint8_t level) {
    Buddy *new;

    uint8_t buddyState = BUDDY_STATE(table, level, addr);
    if (buddyState) {
        new = NULL;
        PRINT_ERR("Merge not implemented yet");
        CRIT_HLT();
    } else {
        new = grabUsableBuddy(table);
        new->start = addr;
    }

    // Insert buddy
    Buddy *buddy = table->levels[level].list;
    Buddy **pbuddy = &buddy;
    // Skip buddies until it is the right pos to insert the new buddy
    for (; *pbuddy && (*pbuddy)->next && (*pbuddy)->next->start < new->start ;
        pbuddy = &(*pbuddy)->next);

    // Insert the new one
    Buddy *tmp = *pbuddy;
    *pbuddy = new;
    new->next = tmp;
}

__attribute_no_vectorize__
void initBuddy(EfiMemMap *physMemMap) {
    MemoryRange validMemory[256];
    uint8_t validCount = getValidMemRanges(physMemMap, &validMemory);

    int16_t where = -1;
    uint64_t totalSize = 0;
    PhysAddr memoryChunk = 0;
    for (uint8_t i = 0; i < validCount; i++) {
        memoryChunk = ALIGN(validMemory[i].start, 1<<21);
        totalSize = memoryChunk - validMemory[i].start + (2 << 20);
        if (validMemory[i].size >= totalSize) {
            where = i;
            break;
        }
    }

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

    // for (int i = 0; i < validCount; i++) {
    //     uint64_t nbOfPages = validMemory[i].size / (1 << 12);
    //     (void)nbOfPages;
    // }
}

