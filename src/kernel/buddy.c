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
        PhysAddr tmp[BUDDY_MAX_ORDER] = { 0 };
        unsigned level = 0;
        while (level < BUDDY_MAX_ORDER && !src->levels[level].list) level++;
        (void)tmp; (void)level;
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
        // insert big one as its first half one level down
        uint64_t addr = levels[curLevel].list->start;
        BUDDY_TOGGLE_BIT(table, curLevel, addr);
        buddyTransfer(&levels[curLevel].list, &levels[curLevel-1].list);
        
        // insert its second half
        Buddy *tmp = grabUsableBuddy(table);
        tmp->next = levels[curLevel - 1].list;
        levels[curLevel - 1].list = tmp;
        tmp->start = addr + (1 << ((curLevel - 1) + 12));
        curLevel--;
    }
    // we are sure that we've got memory and grabUsableBuddy handles 
    // ooms on its own so we can assume levels[level].list != NULL
    BUDDY_TOGGLE_BIT(table, level, levels[level].list->start);
    return buddyTransfer(&levels[level].list, &table->usable);
}

inline static Buddy *grabAssociatedBuddy(BuddyTable *table, uint8_t level, PhysAddr addr) {
    PhysAddr nextLevelAlignedAddr = addr >> (level + 12 + 1);
    Buddy **buddy = &table->levels[level].list;
    for (; *buddy; buddy = &(*buddy)->next) {
        PhysAddr current = addr >> (level + 12 + 1);
        // kprintf("Cur: 0x%X, target: 0x%X\n", current, nextLevelAlignedAddr);
        if (current == nextLevelAlignedAddr) {
            break;
        }
    }
    if (!*buddy) {
        kprintf("level = %u, addr = 0x%X\n", (unsigned)level, addr);
        kputs("map :");
        for (int i = 0; i < 10; i++) {
            kprintf(" 0x%x", ((uint8_t *)table->levels[level].map)[i]);
        }
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
    uint8_t done = 0;
    while (!done) {
        // read pair state, then either we insert (0 -> 1) or we merge (1 -> 0) so we can just flip
        uint8_t buddyState = BUDDY_STATE(table, level, addr);
        BUDDY_TOGGLE_BIT(table, level, addr);

        // if we have to merge
        if (buddyState && level != BUDDY_MAX_ORDER) {
            // grab and discard taddr's buddy
            new = grabAssociatedBuddy(table, level, addr);
            new->next = table->usable;
            table->usable = new;
            
            // set addr to the pair's address and raise level
            addr = ALIGN(addr, 1 << (level + 12)); // Align to the next level
            level++;
        }
        // if we have to insert
        else {
            new = grabUsableBuddy(table);
            new->start = addr;

            // Insert new
            new->next = table->levels[level].list;
            table->levels[level].list = new;

            done = 1;
        }
    }
}

void initBuddy(EfiMemMap *physMemMap) {
    uint64_t totalRAM = getTotalRAM(physMemMap);
    buddyTable.totalRAM = totalRAM;
    kprintf("Total RAM: %U\n", totalRAM);
    if (totalRAM >= (1UL<<40)) {
        PRINT_ERR("HOW MUCH RAM DO YOU HAVE ??????\n");
        CRIT_HLT();
    }

    MemoryRange validMemory[256];
    uint8_t validCount = getValidMemRanges(physMemMap, &validMemory);

    PhysAddr memoryChunk = _getPhysMemoryFromMemRanges(&validMemory, &validCount, 1<<21);
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
    uint16_t pdIdx = 1, ptIdx = 0;
    for (uint8_t level = 0; level < BUDDY_MAX_ORDER; level++) {
        uint64_t neededPages = (totalRAM >> (
            12 +    // group ram bytes by page
            1 +     // group buddy pages by pair
            level + // each levels requires 2 times less bits
            3 +     // bits -> bytes
            12      // bytes -> pages
        )) + 1;

        kprintf("level: %U, neededPages: %U\n", level, neededPages);
        // Allocate and map each needed pages
        // Map the memory as PT entries (4KiB pages)
        for (uint64_t i = 0; i < neededPages; i++) {
            // Need to create a new PT
            // kprintf("pd: %u, pt: %u\n", pdIdx, ptIdx);
            if (ptIdx == 0) {
                if (pdIdx > 510) {
                    PRINT_ERR("ERRM THIS SHOULDN'T HAPPEN\n");
                    CRIT_HLT();
                } else {
                    PhysAddr physPage = _getPhysMemoryFromMemRanges(&validMemory, &validCount, 1 << 12);
                    clearPageTable(physPage);
                    ((PageEntry *)PD(510, 508))[pdIdx].whole = MAKE_PAGE_ENTRY(physPage, PTE_P | PTE_RW | PTE_NX);
                    invlpg((uint64_t)VA(510, 508, pdIdx, ptIdx));
                    // kprintf("New pt, pdIdx: %u, pa: 0x%X, va: 0x%X\n", pdIdx, physPage, VA(510, 508, pdIdx, 0));
                }
            }

            // Map a physical page
            PhysAddr page = _getPhysMemoryFromMemRanges(&validMemory, &validCount, 1 << 12);
            ((PageEntry *)PT(510, 508, pdIdx))[ptIdx].whole = MAKE_PAGE_ENTRY(page, PTE_P | PTE_RW | PTE_NX);
            invlpg((uint64_t)VA(510, 508, pdIdx, ptIdx));

            // Clear the whole page
            memset(VA(510, 508, pdIdx, ptIdx), 0, 1<<12);

            ptIdx++;
            // If the page table is full go to next index of pd
            if (ptIdx > 511) { ptIdx = 0; pdIdx++; }
        }
    }

    for (int i = 0; i < validCount; i++) {
        uint64_t nbOfPages = validMemory[i].size / (1 << 12);
        for (uint64_t j = 0; j < nbOfPages; j++) {
            buddyFree(&buddyTable, 0, validMemory[i].start + j * (1 << 12));
        }
    }
}

