#include "buddy.h"
#include "asm.h"
#include "kterm.h"
#include "attribute.h"
#include "kmemory.h"

static BuddyTable buddyTable = {0};

static PhysAddr buddyTransfer(Buddy **src, Buddy **dest) {
    if (!src || !*src) return 1;
    Buddy *tmp = *src;
    *src = (*src)->next;
    tmp->next = *dest;
    *dest = tmp;
    return 0;
}

// THOU SHALL NOT FAIL
static Buddy *grabUsableBuddy(BuddyTable *src) {
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
        PRINT_ERR("[FATAL] OOM :p\n");
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

inline static Buddy *getAssociatedBuddy(BuddyTable *table, uint8_t level, PhysAddr addr) {
    register PhysAddr nextLevelAlignedAddr = ALIGN(addr, 1 << (level + 1 + 12));
    register Buddy **buddy = &table->levels[level].list;
    for (; *buddy; buddy = &(*buddy)->next) {
        if (ALIGN((*buddy)->start, 1 << (level + 1 + 12)) == nextLevelAlignedAddr) {
            break;
        }
    }
    if (!*buddy) {
        PRINT_ERR("Buddy is gone...\n");
        CRIT_HLT();
    }
    Buddy *ret = *buddy;
    *buddy = (*buddy)->next;
    return ret;
}

void buddyFree(BuddyTable *table, uint8_t level, PhysAddr addr) {
    addr = ALIGN(addr, 1 << (level + 12)); // just in case
    Buddy *new;

    uint8_t buddyState = BUDDY_STATE(table, level, addr);
    if (buddyState) {
        new = getAssociatedBuddy(table, level, addr);
        level++;
        new->start = ALIGN(addr, 1 << (level + 12)); // Align to the next level
    } else {
        new = grabUsableBuddy(table);
        new->start = addr;
        BUDDY_TOGGLE_BIT(table, level, addr);
    }

    // Insert buddy
    register Buddy **buddy = &table->levels[level].list;
    // Skip buddies until it is the right pos to insert the new buddy
    for (; *buddy && (*buddy)->next && (*buddy)->next->start < new->start ;
        buddy = &(*buddy)->next);
    Buddy *tmp = *buddy;
    *buddy = new;
    new->next = tmp;
}

void initBuddy(EfiMemMap *physMemMap) {
    MemoryRange validMemory[256];
    uint8_t validCount = getValidMemRanges(physMemMap, &validMemory);

    PhysAddr memoryChunk = _getPhysMemoryFromMemRanges(&validMemory, validCount, 1<<21);
    ((PageEntry *)PD(510, 508))[0].whole = MAKE_PAGE_ENTRY(memoryChunk, PTE_P | PTE_RW | PTE_PS | PTE_NX);
    Buddy *buf = (Buddy *)VA(510, 508, 0, 0);
    invlpg((uint64_t)buf);

    // Linked list init for the usable buddies :)
    uint64_t count = (1 << 20) / sizeof(Buddy);
    for (uint64_t i = 0; i < (count - 1); i++) {
        buf[i].next = &buf[i+1];
    }
    buf[count - 1].next = NULL;
    buddyTable.usable = buf;

    // Init buddy map for each levels
    // Get the max physical memory address to get the count of uint64_t needed for each level
    PhysAddr max = validMemory[validCount].start + validMemory[validCount].size;
    for (uint8_t level = 0; level < BUDDY_MAX_ORDER; level++) {
        uint64_t needed = max >> (level + 12);
        (void)needed;
    }

    // for (int i = 0; i < validCount; i++) {
    //     uint64_t nbOfPages = validMemory[i].size / (1 << 12);
    //     for (uint64_t j = 0; j < nbOfPages; j++) {
    //         buddyFree(&buddyTable, 0, validMemory[i].start + j * (1 << 12));
    //     }
    // }
}

