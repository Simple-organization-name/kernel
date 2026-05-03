#include <buddy.h>
#include <asm.h>
#include <kterm.h>
#include <memmap.h>
#include <kmemory.h>

static BuddyTable   _buddyTable = {0};
static PhysAddr     _reservedBuddyForReplenishing = 0; // Yep

#define BUDDY_SIZE(level)               ((1UL << (level)) * (1UL << 12))
#define BUDDY_PAIR_ID(level, addr)      ((addr) >> ((level) + 12 + 1))
#define BUDDY_STATE(level, addr)        (_buddyTable.levels[level].map[BUDDY_PAIR_ID(level, addr) / 64] &  (1ULL << (BUDDY_PAIR_ID(level, addr) % 64)))
#define BUDDY_SET_BIT(level, addr)      (_buddyTable.levels[level].map[BUDDY_PAIR_ID(level, addr) / 64] |= (1ULL << (BUDDY_PAIR_ID(level, addr) % 64)))
#define BUDDY_REMOVE_BIT(level, addr)   (_buddyTable.levels[level].map[BUDDY_PAIR_ID(level, addr) / 64] &= ~(1ULL << (BUDDY_PAIR_ID(level, addr) % 64)))
#define BUDDY_TOGGLE_BIT(level, addr)   (_buddyTable.levels[level].map[BUDDY_PAIR_ID(level, addr) / 64] ^= (1ULL << (BUDDY_PAIR_ID(level, addr) % 64)))

static PhysAddr buddyTransfer(Buddy **src, Buddy **dest) {
    if (!src || !*src) return ADDR_MAX;
    Buddy *tmp = *src;
    *src = (*src)->next;
    tmp->next = *dest;
    *dest = tmp;
    return tmp->start;
}

// Uses only 2MiB pages
static void initBuddyChainedList(Buddy *buf) {
    // Linked list init for the usable buddies :)
    uint64_t count = (1UL << 21) / sizeof(Buddy);
    for (uint64_t i = 0; i < (count - 1); i++) {
        buf[i].next = &buf[i+1];
    }
    buf[count - 1].next = NULL;
}

static Buddy *grabUsableBuddy(BuddyTable *src) {
    if (!src->usable) {
        PhysAddr addr = _reservedBuddyForReplenishing;
        uint16_t idx[4] = {510, 508, 1, 0};
        int found = findEmptySlotPageIdx(PTE_PD, idx); // OOPS CYCLIC DEPENDENCY
        if(!found || idx[1] != 508) {
            PRINT_ERR("OOUB (Out-Of-Usable-Buddy)\n");
            PRINT_ERR("Failed to find empty space in mem management PD for new usable buddies\n");
            CRIT_HLT();
        }
        mapPage(idx, PTE_PD, addr, PTE_PS | PTE_RW | PTE_NX);
        Buddy *buf = VA_ARRAY(idx);
        initBuddyChainedList(buf);
        _buddyTable.usable = buf;
        // We replenished the usable buddies, so we can call buddyAlloc safely
        _reservedBuddyForReplenishing = buddyAlloc(BUDDY_2M);
    }
    Buddy *out = src->usable;
    src->usable = src->usable->next;
    out->next = NULL;
    return out;
}

PhysAddr buddyAlloc(uint8_t level) {
    uint8_t curLevel;
    for (curLevel = level; curLevel < BUDDY_MAX_ORDER && !_buddyTable.levels[curLevel].list; curLevel++); // find nearest usable buddy iykyk
    if (curLevel == BUDDY_MAX_ORDER) {
        PRINT_ERR("[FATAL] OOM :p\n");
        CRIT_HLT();
    }
    while (curLevel != level) {
        // insert big one as its first half one level down
        uint64_t addr = _buddyTable.levels[curLevel].list->start;
        BUDDY_TOGGLE_BIT(curLevel, addr);
        buddyTransfer(&_buddyTable.levels[curLevel].list, &_buddyTable.levels[curLevel-1].list);

        // insert its second half
        Buddy *tmp = grabUsableBuddy(&_buddyTable);
        tmp->next = _buddyTable.levels[curLevel - 1].list;
        _buddyTable.levels[curLevel - 1].list = tmp;
        tmp->start = addr + (1U << ((curLevel - 1) + 12));
        curLevel--;
    }
    // we are sure that we've got memory and grabUsableBuddy handles
    // ooms on its own so we can assume levels[level].list != NULL
    // PRINT_DEBUG("level=%u, pa=%p\n", level, _buddyTable.levels[level].list->start);
    // uint64_t id = BUDDY_PAIR_ID(level, _buddyTable.levels[level].list->start);
    // PRINT_DEBUG("pair id: %U\n", id);
    // PRINT_DEBUG("buddy state: 0x%X\n", BUDDY_STATE(level, _buddyTable.levels[level].list->start));
    // PRINT_DEBUG("buddy bit in the map: %p\n", &_buddyTable.levels[level].map);
    BUDDY_TOGGLE_BIT(level, _buddyTable.levels[level].list->start);
    return buddyTransfer(&_buddyTable.levels[level].list, &_buddyTable.usable);
}

inline static Buddy *grabAssociatedBuddy(uint8_t level, PhysAddr addr) {
    PhysAddr nextLevelAlignedAddrHead = addr >> (level + 12 + 1);
    Buddy **buddy = &_buddyTable.levels[level].list;
    for (; *buddy; buddy = &(*buddy)->next) {
        PhysAddr current = addr >> (level + 12 + 1);
        if (current == nextLevelAlignedAddrHead) {
            break;
        }
    }
    if (!*buddy) {
        PRINT_ERR("Buddy is gone...\n");
        PRINT_ERR("level = %u, addr = %p\n", (unsigned)level, addr);
        PRINT_ERR("map (first bytes):");
        for (int i = 0; i < 10; i++) {
            kprintf(" 0x%x", ((uint8_t *)_buddyTable.levels[level].map)[i]);
        }
        kputc('\n');
        CRIT_HLT();
    }
    Buddy *ret = *buddy;
    *buddy = (*buddy)->next;
    return ret;
}

void buddyFree(uint8_t level, PhysAddr addr) {
    // PRINT_DEBUG("Called buddyFree %p, is level %u\n", addr, level);
    if (level >= BUDDY_MAX_ORDER) {
        PRINT_ERR("WTF");
        CRIT_HLT();
    }
    addr = ALIGN(addr, 1U << (level + 12)); // just in case
    if (level < BUDDY_MAX_ORDER - 1 && BUDDY_STATE(level, addr)) {
        // need to seek and merge
        Buddy *friend = grabAssociatedBuddy(level, addr);
        // PRINT_DEBUG("Found friend with phys addr: %p\n", friend->start);
        BUDDY_REMOVE_BIT(level, addr);
        // PRINT_DEBUG("Transfer friend struct (%p) to usable structs\n", friend);
        buddyTransfer(&friend, &_buddyTable.usable);
        buddyFree(level + 1, ALIGN(addr, 1U << (level + 1 + 12)));
    } else {
        // need to insert
        // PRINT_DEBUG("Friend not found (supposed to be the pair (%p, %p)), inserting\n", ALIGN(addr, 1 << (level + 1 + 12)), ALIGN(addr, 1 << (level + 1 + 12)) + (1 << (level + 12)));
        buddyTransfer(&_buddyTable.usable, &_buddyTable.levels[level].list);
        _buddyTable.levels[level].list->start = addr;
        BUDDY_SET_BIT(level, addr);
    }
}

static void initBuddyMap(EfiMemMap *map) {
    // Init buddy map for each levels
    uint16_t pdIdx = 1, ptIdx = 0;
    for (uint8_t level = 0; level < BUDDY_MAX_ORDER; level++) {
        uint64_t neededPages = (_buddyTable.totalRAM >> (
            12 +    // group ram bytes by page
            1 +     // group buddy pages by pair
            level + // each levels requires 2 times less bits
            3 +     // bits -> bytes
            12      // bytes -> pages
        )) + 1;

        // Allocate and map each needed pages
        // Map the memory as PT entries (4KiB pages)
        for (uint64_t i = 0; i < neededPages; i++) {
            // Need to create a new PT
            if (ptIdx == 0) {
                if (pdIdx > 510) {
                    PRINT_ERR("ERRM THIS SHOULDN'T HAPPEN\n");
                    CRIT_HLT();
                } else {
                    PhysAddr physPage = _getPhysMemoryFromEFIMemMap(map, 1);
                    if (physPage == ADDR_MAX) {
                        PRINT_ERR("Failed to get memory for buddy map\n");
                        CRIT_HLT();
                    }
                    clearPageTable(physPage);
                    const uint16_t idx[] = {510, 508, pdIdx, 0};
                    if (!mapPage(idx, PTE_PD, physPage, PTE_RW | PTE_NX)) {
                        PRINT_ERR("welp");
                        CRIT_HLT();
                    }
                    // ((PageEntry *)PD(510, 508))[pdIdx].whole = MAKE_PAGE_ENTRY(physPage, PTE_RW | PTE_NX);
                    // PRINT_DEBUG("New pt, pdIdx: %u, pa: %p, va: %p\n", pdIdx, physPage, VA(510, 508, pdIdx, 0));
                }
            }

            // Map a physical page
            PhysAddr page = _getPhysMemoryFromEFIMemMap(map, 1);
            if (page == ADDR_MAX) {
                PRINT_ERR("Faild to get memory for buddy map\n");
                CRIT_HLT();
            }
            const uint16_t idx[] = {510, 508, pdIdx, ptIdx};
            if (!mapPage(idx, PTE_PT, page, PTE_RW | PTE_NX)) {
                PRINT_ERR("welp");
                CRIT_HLT();
            }
            
            if (i == 0) {
                // ((PageEntry *)PT(510, 508, pdIdx))[ptIdx].whole = MAKE_PAGE_ENTRY(page, PTE_RW | PTE_NX);
                uint64_t *addr = VA(510, 508, pdIdx, ptIdx);
                // flushTLB((uint64_t)addr);
                _buddyTable.levels[level].map = addr;
            }

            // Clear the whole page
            memset(VA(510, 508, pdIdx, ptIdx), 0, 1U<<12);

            ptIdx++;
            // If the page table is full go to next index of pd
            if (ptIdx > 511) { ptIdx = 0; pdIdx++; }
        }
    }
}

void initBuddy(EfiMemMap *physMemMap) {
    uint64_t totalRAM = getTotalRAM(physMemMap);
    PRINT_DEBUG("Total RAM: %UB\n", totalRAM);
    if (totalRAM >= (1UL<<40)) {
        PRINT_ERR("HOW MUCH RAM DO YOU HAVE ??????\n");
        CRIT_HLT();
    }
    _buddyTable.totalRAM = totalRAM;

    // Get memory to make usable buddies
    PhysAddr memoryChunk = _getPhysMemoryFromEFIMemMap(physMemMap, 1U<<(21 - 12));
    const uint16_t idx[] = {510, 508, 0, 0};
    if (!mapPage(idx, PTE_PD, memoryChunk, PTE_RW | PTE_PS | PTE_NX)) {
        PRINT_ERR("welp");
        CRIT_HLT();
    }
    // ((PageEntry *)PD(510, 508))[0].whole = MAKE_PAGE_ENTRY(memoryChunk, PTE_P | PTE_RW | PTE_PS | PTE_NX);
    Buddy *buf = (Buddy *)VA(510, 508, 0, 0);
    flushTLB((uint64_t)buf);
    // Make the buddies
    initBuddyChainedList(buf);
    _buddyTable.usable = buf;

    // Init buddy map
    initBuddyMap(physMemMap);

    // Init buddy table with all the available memory
    for (uint64_t i = 0; i < physMemMap->count; i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemMap->map + physMemMap->descSize * i);
        if (!isValidMem(desc)) continue;
        // PRINT_DEBUG("Found valid memory descriptor: type: %U, pa: %p, no of pages: %U\n", desc->Type, desc->PhysicalStart, desc->NumberOfPages);

        // BuddyType startLevel = BUDDY_4K;
        // for (int level = BUDDY_4K; level < BUDDY_MAX_ORDER; level++) {
        //     if (ALIGN(desc->PhysicalStart, BUDDY_IDX_BLOCK_SIZE(level)) == desc->PhysicalStart) {
        //         startLevel = level;
        //     }
        // }
        // here, `startLevel` is the max level of buddy i can insert without breaking alignment



        // // for (int level = BUDDY_MAX_ORDER - 1; level >= 0; level--) {
        // //     const uint64_t levelPages = 1 << level;
        // //     while (desc->NumberOfPages >= levelPages) {
        // //         buddyFree(level, desc->PhysicalStart);
        // //         desc->PhysicalStart += levelPages * 4096;
        // //         desc->NumberOfPages -= levelPages;
        // //     }
        // // }
        for (uint64_t j = 0; j < desc->NumberOfPages; j++) {
            // kprintf("(%U, %U) ", i, j);
            buddyFree(BUDDY_4K, desc->PhysicalStart + j * 4096);
        }
    }

    _reservedBuddyForReplenishing = buddyAlloc(BUDDY_2M); // reserve a page of 2MiB
}

void printBuddyTableInfo() {
    kprintf("\n   ----==== Buddy Table infos ====----   \n");
    for (uint8_t i = 0; i < BUDDY_MAX_ORDER; i++) {
        kprintf("Level %u informations: \n", i);
        volatile uint64_t buddyCount = 0;
        for (Buddy *bud = _buddyTable.levels[i].list; bud; bud = bud->next) buddyCount++;
        uint64_t neededPages = (_buddyTable.totalRAM >> (
            12 +    // group ram bytes by page
            1 +     // group buddy pages by pair
            i +     // each levels requires 2 times less bits
            3 +     // bits -> bytes
            12      // bytes -> pages
        )) + 1;
        kprintf(
            "   Number of buddies: %U | Map start: [pa: %p, va: %p]\n"
            "   Theoric needed 4K pages nb: %U | Theoric max number of buddies: %U\n",
            buddyCount, 
            getMapping((VirtAddr)_buddyTable.levels[i].map, NULL), _buddyTable.levels[i].map, 
            neededPages, _buddyTable.totalRAM >> (12 + i)
        );
    }
    kputc('\n');
}
