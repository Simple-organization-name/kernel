#include "kmemory.h"
#include "asm.h"
#include "memmap.h"
#include "buddy.h"

void clearPageTable(PhysAddr addr) {
    (PT(510, 508, 511))[0].whole = MAKE_PAGE_ENTRY(addr, PTE_NX | PTE_RW);
    memset(VA(510, 508, 511, 0), 0, 4096);
    (PT(510, 508, 511))[0].whole = 0;
    invlpg((uint64_t)VA(510, 508, 511, 0));
}

inline static PageEntry *getTable(PageType type, uint16_t *idx) {
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

static int _findEmptySlotPageIdx(uint8_t targetType, uint16_t *idx, uint8_t curType) {
    PageEntry *table = getTable(curType, idx);

    for (uint16_t i = idx[curType]; i < (curType == PTE_PML4 ? 511 : 512); i++) {
        // kprintf("targetType: %u, curType: %u, curIdx: %u\n", targetType, curType, i);
        idx[curType] = i;
        if (curType == targetType && !table[i].sreserved) { // if it is the right level and the slot is free
            table[i].whole = PTE_RESERVED;
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

inline int findEmptySlotPageIdx(uint8_t targetType, uint16_t *idx) {
    return _findEmptySlotPageIdx(targetType, idx, PTE_PML4);
}

static size_t _findEmptyRangePageIdx(uint8_t targetType, uint16_t *idx, size_t count, uint8_t curType, uint16_t *curIdx, size_t found) {
    kprintf("found: %U\n", found);
    // kprintf("idx: %u %u %u %u, curIdx: %u %u %u %u\n", idx[0], idx[1], idx[2], idx[3], curIdx[0], curIdx[1], curIdx[2], curIdx[3]);
    // Get the page table associated with the current type at the current index in the paging
    PageEntry *table = getTable(curType, curIdx);

    // Set the max index, if it is PML3 set it to 511 as the 511th page is the recursive mapping
    uint16_t maxIdx = curType == PTE_PML4 ? 511 : 512;
    for (uint16_t *i = &curIdx[curType]; *i < maxIdx; (*i)++) {
        // Cases where the page is occupied and we should slide the window
        if (table[*i].pageSize || // If the page is actually mapped to physical memory
            table[*i].sreserved || // If the page is reserved with special use cases, cannot be touched by normal functions
            table[*i].reserved || // If the page is already reserved
            (table[*i].present && curType == targetType) // If the page is mapped at level PTE_PT as PTs doesn't have the PS flag
        ) {
            // kprintf("Occupied page, PS: %u, SRES: %u, RES: %u, P: %u\n", table[*i].pageSize, table[*i].sreserved, table[*i].reserved, targetType == curType && table[*i].present);
            // kprintf("targetType: %u, curType: %u, curIdx: %u %u %u %u\n", targetType, curType, curIdx[0], curIdx[1], curIdx[2], curIdx[3]);
            found = 0; // reset found to 0, as the next empty slot will not be contiguous to the current range
            memcpy(idx, curIdx, sizeof(uint16_t) * 4); // Change the return idx to the current one
            idx[curType]++;
            for (uint8_t j = curType + 1; j <= targetType; j++)
                idx[j] = 0;
            continue;
        }
        if (targetType != curType) { // If the current level is not the targeted level
            if (!table[*i].present) { // If the page isn't present
                kprintf("add %U pages\n", (size_t)(512 * (1<<(9 * (curType - targetType)))));
                found += 512 * (1<<(9 * (curType - targetType)));
            } else {
                // kprintf("Start recursion at %u %u %u %u, level: %u\n", curIdx[0], curIdx[1], curIdx[2], curIdx[3], curType + 1);
                found = _findEmptyRangePageIdx(targetType, idx, count, curType + 1, curIdx, found); // Recurse in the next level
                curIdx[curType + 1] = 0;
                // kprintf("Found after recursion: %u, starting at: %u %u %u %u\n", found, idx[0], idx[1], idx[2], idx[3]);
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

static size_t createNeededTable(uint8_t targetType, uint16_t *curIdx, size_t count, size_t found, uint8_t curType) {
    PageEntry *table = getTable(curType, curIdx);
    uint16_t maxIdx = curType == PTE_PML4 ? 511 : 512;
    for (uint16_t *i = &curIdx[curType]; *i < maxIdx; (*i)++) {
        if (table[*i].present) continue;
        if (targetType != curType) {
            PhysAddr page = buddyAlloc(BUDDY_4K);
            clearPageTable(page);
            if (!mapPage(curIdx, curType, page, PTE_RW)) {
                PRINT_ERR("Failed to map a page\n");
                CRIT_HLT();
            }
            found = createNeededTable(targetType, curIdx, count, found, curType + 1);
            curIdx[curType + 1] = 0;
        } else {
            table[*i].reserved = 1;
            found++;
        }
        if (count == found)
            return found;
    }

    return found;
}

/**
 * \param targetType The type of page needed
 * \param idx The array of 4 uint16_t (`uint16_t[4]`) where to store the start of the range if found
 * \param count The number of pages needed
 * \return The number of pages if successful, 0 else
 */
inline size_t findEmptyRangePageIdx(uint8_t targetType, uint16_t *idx, size_t count) {
    uint16_t curIdx[4];
    memcpy(curIdx, idx, sizeof(uint16_t) * 4);
    size_t found = _findEmptyRangePageIdx(targetType, idx, count, PTE_PML4, curIdx, 0);
    PRINT_WARN("found: %U\n", found);
    if (found < count) return 0;

    memcpy(curIdx, idx, sizeof(uint16_t) * 4);
    size_t mappedPages = createNeededTable(targetType, curIdx, count, 0, 0);
    PRINT_WARN("mapped pages: %U\n", mappedPages);
    if (mappedPages != count) {
        // TODO: NEED TO CLEAR THE RESERVED FLAGS OF THE ALREADY MAPPED PAGES
        return 0;
    }

    return count;
}

inline int mapPage(uint16_t *idx, uint8_t pageType, PhysAddr addr, uint64_t flags) {
    PageEntry *table = getTable(pageType, idx);
    PageEntry *entry = &table[idx[pageType]];
    if (entry->sreserved || entry->present) {
        PRINT_WARN("Page used: SRES: %d, P: %d, abort mapping\n", entry->sreserved, entry->present);
        return 0;
    }
    table[idx[pageType]].whole = MAKE_PAGE_ENTRY(addr, flags);
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
    entry->whole = 0;
    invlpg(virt);
    return 1 + entry->present;
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
    if (table[idx[pageType]].sreserved || table[idx[pageType]].present) return 0;
    table[idx[pageType]].whole = PTE_SRESERVED;
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
