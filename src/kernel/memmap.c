#include "kmemory.h"
#include "asm.h"
#include "memmap.h"
#include "buddy.h"

void clearPageTable(PhysAddr addr) {
    (PT(510, 508, 511))[0].whole = MAKE_PAGE_ENTRY(addr, PTE_P | PTE_NX | PTE_RW);
    invlpg((uint64_t)VA(510, 508, 511, 0));
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
        if (curType == targetType && !table[i].reserved) // if it is the right level and the slot is free
            return 1;

        else if (curType != targetType && !table[i].pageSize) {
            if (!table[i].reserved) {
                PhysAddr page = buddyAlloc(0);
                table[i].whole = MAKE_PAGE_ENTRY(page, PTE_RW);
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
    PageEntry *table = getTable(curType, curIdx);

    uint16_t maxIdx = curType == PTE_PML4 ? 511 : 510;
    for (uint16_t i = curIdx[curType]; i < maxIdx; i++) {
        curIdx[curType] = i;
        
        // Cases where the page is occupied and we should slide the window
        if (table[i].pageSize || table[i].reserved || (targetType == curType && table[i].present)) {
            // kprintf("Occupied page\n");
            found = 0;
            memcpy(idx, curIdx, sizeof(uint16_t) * 4);
            idx[curType] = i+1;
            // kprintf("Ajusting new idx to %u %u %u %u\n", idx[0], idx[1], idx[2], idx[3]);
            continue;
        }

        if (targetType != curType) {
            if (!table[i].present) {
                found += (targetType - curType) * 512;
                kprintf("Page not present adding %lu pages\n", (targetType - curType) * 512);
            } else {
                found = _findEmptyRangePageIdx(targetType, idx, count, curType + 1, curIdx, found);
            }
        } else {
            found++;
        }
        if (found >= count) {
            return found;
        }
    }

    curIdx[curType] = 0;
    return found;
}

inline int findEmptyRangePageIdx(uint8_t targetType, uint16_t *idx, uint16_t count) {
    uint16_t curIdx[4];
    memcpy(curIdx, idx, sizeof(uint16_t) * 4);
    size_t found = _findEmptyRangePageIdx(targetType, idx, count, PTE_PML4, curIdx, 0);
    kprintf("Found a total of %U pages\n", found);
    if (found < count) return 0;

    return found;
}

inline int mapPage(uint16_t *idx, uint8_t pageType, PhysAddr addr, uint64_t flags) {
    PageEntry *table = getTable(pageType, idx);
    if (table[idx[pageType]].reserved) return 1;
    table[idx[pageType]].whole = MAKE_PAGE_ENTRY(addr, flags);
    invlpg((uint64_t)VA_ARRAY(idx));
    return 0;
}

int unmapPage(VirtAddr virtual) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    PageEntry *entry = PML4() + pml4_index;
    if (!entry->present) return 0;

    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = PDPT(pml4_index) + pdpt_index;
    if (!entry->present) return 0;
    if (entry->pageSize) {
        entry->whole = 0;
        invlpg(virtual);
        return 1;
    }

    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = PD(pml4_index, pdpt_index) + pd_index;
    if (!entry->present) return 0;
    if (entry->pageSize) {
        entry->whole = 0;
        invlpg(virtual);
        return 1;
    }

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = PT(pml4_index, pdpt_index, pd_index) + pt_index;
    if (!entry->present) return 0;
    entry->whole = 0;
    invlpg(virtual);
    return 1;
}

int reservePage(VirtAddr addr, PageType level)
{
    uint16_t idx[4] = {
        (addr >> 39) & 0x1FF,
        (addr >> 30) & 0x1FF,
        (addr >> 21) & 0x1FF,
        (addr >> 12) & 0x1FF
    };
    PageEntry *table = getTable(level, idx);
    if (table[idx[level]].reserved) return 1;
    table[idx[level]].whole = PTE_RESERVED;
    return 0;
}

int unReservePage(VirtAddr addr)
{
    uint16_t idx[4] = {
        (addr >> 39) & 0x1FF,
        (addr >> 30) & 0x1FF,
        (addr >> 21) & 0x1FF,
        (addr >> 12) & 0x1FF
    };
    if (PML4()[idx[0]].reserved) {
        PML4()[idx[0]].whole = 0;
        return 1;
    } else if (PDPT(idx[0])[idx[1]].reserved) {
        PDPT(idx[0])[idx[1]].whole = 0;
        return 1;
    } else if (PD(idx[0],idx[1])[idx[2]].reserved) {
        PD(idx[0],idx[1])[idx[2]].whole = 0;
        return 1;
    } else if (PT(idx[0], idx[1], idx[2])[idx[3]].reserved) {
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
