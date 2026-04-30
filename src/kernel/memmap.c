#include <kmemory.h>
#include <asm.h>
#include <buddy.h>
#include <kerror.h>
#include <memmap.h>

inline void clearPageTable(const PhysAddr addr) {
    static const uint16_t idx[] = {510, 508, 511, 0};
    static const uint64_t virt = (uint64_t)VA_ARRAY(idx);
    mapPage(idx, PTE_PT, addr, PTE_NX | PTE_RW);
    memset((void *)virt, 0, 4096);
    unmapPage(virt, NULL);
}

inline static PageEntry *getTable(const PageType type, const uint16_t * const idx) {
    PageEntry *table;
    switch (type) {
        case PTE_PML4: // Searching for entry in pml4
            table = PML4();
            break;
        case PTE_PDP: // ... for entry in pdp
            table = PDPT(idx[0]);
            break;
        case PTE_PD: // ... for entry in pd
            table = PD(idx[0], idx[1]);
            break;
        case PTE_PT: // ... for entry in pt
            table = PT(idx[0], idx[1], idx[2]);
            break;
        default:
            PRINT_ERR("Invalid level\n");
            CRIT_HLT();
    }
    return table;
}

static int _findEmptySlotPageIdx(const uint8_t targetType, uint16_t * const idx, const uint8_t curType) {
    PageEntry * const table = getTable(curType, idx);

    for (uint16_t i = idx[curType]; i < (curType == PTE_PML4 ? 511 : 512); i++) {
        // kprintf("targetType: %u, curType: %u, curIdx: %u\n", targetType, curType, i);
        idx[curType] = i;
        if (curType == targetType && !table[i].sreserved) { // if it is the right level and the slot is free
            table[i].whole = PTE_R;
            return 1;
        }

        else if (curType != targetType && !table[i].pageSize) {
            if (!table[i].sreserved) {
                PhysAddr page = buddyAlloc(BUDDY_4K);
                if (!mapPage(idx, curType, page, PTE_RW)) {
                    PRINT_ERR("Failed to map page\n");
                    CRIT_HLT();
                }
            }
            if (_findEmptySlotPageIdx(targetType, idx, curType + 1))
                return 1;
        }
    }

    idx[curType] = 0;
    return 0;
}

inline int findEmptySlotPageIdx(uint8_t targetType, uint16_t * const idx) {
    return _findEmptySlotPageIdx(targetType, idx, PTE_PML4);
}

static size_t _findEmptyRangePageIdx(const uint8_t targetType, uint16_t * const idx, const size_t count, const uint8_t curType, uint16_t * const curIdx, size_t found) {
    // Get the page table associated with the current type at the current index in the paging
    PageEntry * const table = getTable(curType, curIdx);

    // Set the max index, if it is PML3 set it to 511 as the 511th page is the recursive mapping
    const uint16_t maxIdx = curType == PTE_PML4 ? 511 : 512;
    for (uint16_t *i = &curIdx[curType]; *i < maxIdx; (*i)++) {
        // Cases where the page is occupied and we should slide the window
        if (table[*i].pageSize || // If the page is actually mapped to physical memory
            table[*i].sreserved || // If the page is reserved with special use cases, cannot be touched by normal functions
            table[*i].reserved || // If the page is already reserved
            (table[*i].present && curType == targetType) // If the page is mapped at level PTE_PT as PTs doesn't have the PS flag
        ) {
            found = 0; // reset found to 0, as the next empty slot will not be contiguous to the current range
            memcpy(idx, curIdx, sizeof(uint16_t) * 4); // Change the return idx to the current one
            idx[curType]++;
            for (uint8_t j = curType + 1; j <= targetType; j++) // Clear lower page table idxs
                idx[j] = 0;
            continue;
        }
        if (targetType != curType) { // If the current level is not the targeted level
            if (!table[*i].present) { // If the page isn't present
                found += 512 * (1<<(9 * (curType - targetType)));
            } else {
                found = _findEmptyRangePageIdx(targetType, idx, count, curType + 1, curIdx, found); // Recurse in the next level
                curIdx[curType + 1] = 0;
            }
        } else {
            found++;
        }

        if (found >= count) {
            return found;
        }
    }

    return found;
}

/**
 * Create the needed intermediate tables
 * \param targetType Target type of map page
 * \param curIdx The array containing the index where to start the mapping
 * \param count The number of pages wanted by the caller
 * \param curType The current type (recursion)
 */
static size_t createNeededTable(const uint8_t targetType, uint16_t * const curIdx, const size_t count, const uint8_t curType) {
    PRINT_WARN("Called with targetType: %u, curIdx: {%u %u %u %u}, count: %U, curType: %u\n", targetType, curIdx[0], curIdx[1], curIdx[2], curIdx[3], count, curType);
    PageEntry * const table = getTable(curType, curIdx);
    const uint16_t maxIdx = curType == PTE_PML4 ? 511 : 512;
    size_t reserved = 0;
    for (uint16_t *i = &curIdx[curType]; *i < maxIdx; ++*i) {
        if (curType == targetType) {
            // PRINT_WARN("i: %u, &table[*i]: %X\n", *i, &table[*i]); // Page fault here: table[*i] not present
            if (table[*i].present) return 0; // Memory map changed, range is not valid anymore
            table[*i].reserved = 1;
            reserved++;
            // PRINT_WARN("Reserved %U pages\n", reserved);
        } else {
            if (!table[*i].present) {
                const PhysAddr phys = buddyAlloc(BUDDY_4K);
                clearPageTable(phys);
                if (!mapPage(curIdx, curType, phys, PTE_RW)) {
                    PRINT_WARN("Failed to map new page\n"); // Memory map changed, range is not valid anymore
                    return 0;
                }
                // invlpg((uint64_t)VA_ARRAY(curIdx));
                PRINT_WARN("Mapped new table at idx: %u %u %u %u, pa: %X, va: %X, type: %u\n", curIdx[0], curIdx[1], curIdx[2], curIdx[3], phys, VA_ARRAY(curIdx), curType);
                PRINT_WARN("Testing new page...\n");
                PageEntry * const test = getTable(curType + 1, curIdx);
                uint64_t tmp = 0;
                for (uint16_t _ = 0; _ < 512; _++) {
                    tmp += test[_].present;
                }
                PRINT_WARN("Read test successful: read present: %U\n", tmp);
            }
            PRINT_WARN("Recursive call\n");
            reserved += createNeededTable(targetType, curIdx, count - reserved, curType + 1);
            PRINT_WARN("End of recursive call\n");
            // PRINT_WARN("Reserved %U pages\n", reserved);
            curIdx[curType + 1] = 0;
        }

        if (reserved == count) return reserved;
    }

    return reserved;
}

/**
 * \param targetType The type of page needed
 * \param idx The array of 4 uint16_t (`uint16_t[4]`) where to store the start of the range if found
 * \param count The number of pages needed
 * \return The number of pages if successful, 0 else
 */
size_t findEmptyRangePageIdx(const uint8_t targetType, uint16_t *idx, const size_t count) {
    uint16_t curIdx[4];
    memcpy(curIdx, idx, sizeof(uint16_t) * 4);
    const size_t found = _findEmptyRangePageIdx(targetType, idx, count, PTE_PML4, curIdx, 0);
    // PRINT_WARN("found: %U, at: %u %u %u %u\n", found, curIdx[0], curIdx[1], curIdx[2], curIdx[3]);
    if (found < count) return 0;

    memcpy(curIdx, idx, sizeof(uint16_t) * 4);
    const size_t mappedPages = createNeededTable(targetType, curIdx, count, 0);
    // PRINT_WARN("mapped pages: %U\n", mappedPages);
    if (mappedPages != count) {
        // TODO: NEED TO CLEAR THE RESERVED FLAGS OF THE ALREADY MAPPED PAGES
        return 0;
    }

    return count;
}

/**
 * Map a page
 * \param idx The idx array (`uint16_t[4]`) defining the slot in the mapping to use
 * \param pageType The page type to map
 * \param addr The physical address to map
 * \param flags The flags to be used for the mapping, see `memTables.h`
 * \return 1 if success, 0 if there were already a mapped page (`PTE_P`)
 */
inline int mapPage(const uint16_t *idx, uint8_t pageType, PhysAddr addr, uint64_t flags) {
    PageEntry *table = getTable(pageType, idx);
    PageEntry *entry = &table[idx[pageType]];
    if (entry->present) {
        PRINT_WARN("Page used: P: %d, abort mapping\n", entry->present);
        return 0;
    }
    entry->whole = MAKE_PAGE_ENTRY(addr, flags);
    invlpg((uint64_t)VA_ARRAY(idx));
    return 1;
}

// return 0 if failed, 1 if unmapped a mapped physical page,
// 2 if the address was only reserved but not mapped to an actual physical page
int unmapPage(VirtAddr virt, PhysAddr *phys) {
    uint16_t pml4_index = (virt >> 39) & 0x1FF;
    PageEntry *entry = PML4() + pml4_index;
    if (!entry->reserved && !entry->present) return 0;
    if (entry->pageSize) goto unmap;

    uint16_t pdpt_index = (virt >> 30) & 0x1FF;
    entry = PDPT(pml4_index) + pdpt_index;
    if (!entry->reserved && !entry->present) return 0;
    if (entry->pageSize) goto unmap;

    uint16_t pd_index = (virt >> 21) & 0x1FF;
    entry = PD(pml4_index, pdpt_index) + pd_index;
    if (!entry->reserved && !entry->present) return 0;
    if (entry->pageSize) goto unmap;

    uint16_t pt_index = (virt >> 12) & 0x1FF;
    entry = PT(pml4_index, pdpt_index, pd_index) + pt_index;
    if (!entry->reserved && !entry->present) return 0;

unmap:
    *phys = entry->dest;
    bool wasPresent = entry->present;
    entry->whole = 0;
    invlpg(virt);
    return 1 + wasPresent;
}

int sreservePage(VirtAddr addr, PageType pageType)
{
    uint16_t idx[4] = {
        (addr >> 39) & 0x1FF,
        (addr >> 30) & 0x1FF,
        (addr >> 21) & 0x1FF,
        (addr >> 12) & 0x1FF
    };
    PageEntry *table = getTable(pageType, idx);
    if (!PAGE_TABLE_SLOT_AVAILABLE(table[idx[pageType]])) return 0;
    table[idx[pageType]].whole = PTE_SR;
    return 1;
}

int reservePage(VirtAddr addr, PageType pageType)
{
    uint16_t idx[4] = {
        (addr >> 39) & 0x1FF,
        (addr >> 30) & 0x1FF,
        (addr >> 21) & 0x1FF,
        (addr >> 12) & 0x1FF
    };
    PageEntry *table = getTable(pageType, idx);
    if (!PAGE_TABLE_SLOT_AVAILABLE(table[idx[pageType]])) return 0;
    table[idx[pageType]].whole = PTE_R;
    return 1;
}

int unReservePage(VirtAddr addr)
{
    uint16_t idx[4] = {
        (addr >> 39) & 0x1FF,
        (addr >> 30) & 0x1FF,
        (addr >> 21) & 0x1FF,
        (addr >> 12) & 0x1FF
    };
    if (PML4()[idx[0]].sreserved) {
        PML4()[idx[0]].whole = 0;
        return 1;
    } else if (PDPT(idx[0])[idx[1]].sreserved) {
        PDPT(idx[0])[idx[1]].whole = 0;
        return 1;
    } else if (PD(idx[0],idx[1])[idx[2]].sreserved) {
        PD(idx[0],idx[1])[idx[2]].whole = 0;
        return 1;
    } else if (PT(idx[0], idx[1], idx[2])[idx[3]].sreserved) {
        PT(idx[0], idx[1], idx[2])[idx[3]].whole = 0;
        return 1;
    }
    return 0;
}

PhysAddr getMapping(VirtAddr virtual, uint8_t *pageLevel) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    PageEntry entry = PML4()[pml4_index];
    if (!entry.present) return -1;

    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = PDPT(pml4_index)[pdpt_index];
    if (!entry.present) return -1;
    if (entry.pageSize) {
        if (pageLevel) *pageLevel = PTE_PDP;
        return entry.whole & PTE_ADDR;
    }

    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = PD(pml4_index, pdpt_index)[pd_index];
    if (!entry.present) return -1;
    if (entry.pageSize) {
        if (pageLevel) *pageLevel = PTE_PD;
        return entry.whole & PTE_ADDR;
    }

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = PT(pml4_index, pdpt_index, pd_index)[pt_index];
    if (!entry.present) return -1;
    if (pageLevel) *pageLevel = PTE_PT;
    return entry.whole & PTE_ADDR; // pt is always 4KiB so it doesn't have the pageSize flag
}
