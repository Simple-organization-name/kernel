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
            kprintf("Occupied page, PS: %u, SRES: %u, RES: %u, P: %u\n", table[*i].pageSize, table[*i].sreserved, table[*i].reserved, targetType == curType && table[*i].present);
            kprintf("targetType: %u, curType: %u, curIdx: %u %u %u %u\n", targetType, curType, curIdx[0], curIdx[1], curIdx[2], curIdx[3]);
            found = 0; // reset found to 0, as the next empty slot will not be contiguous to the current range
            memcpy(idx, curIdx, sizeof(uint16_t) * 4); // Change the return idx to the current one
            idx[curType]++;
            continue;
        }
        if (targetType != curType) { // If the current level is not the targeted level
            if (!table[*i].present) { // If the page isn't present
                kprintf("Making a new page table, level: %u, curIdx: %u %u %u %u\n", curType + 1, curIdx[0], curIdx[1], curIdx[2], curIdx[3]);
                PhysAddr page = buddyAlloc(BUDDY_4K);
                kprintf("Clearing page (phys: 0x%U).", page);
                clearPageTable(page);
                kprintf(" Done\n");
                // Map a new table
                if (!mapPage(curIdx, curType, page, PTE_RW)) {
                    PRINT_ERR("Failed to map page\n");
                    CRIT_HLT();
                }
                kprintf("New page mapped\n");
            }
            // kprintf("found before recursion: %u\n", found);
            kprintf("Start recursion at %u %u %u %u, level: %u\n", curIdx[0], curIdx[1], curIdx[2], curIdx[3], curType + 1);
            found = _findEmptyRangePageIdx(targetType, idx, count, curType + 1, curIdx, found); // Recurse in the next level
            kprintf("Found after recursion: %u, starting at: %u %u %u %u\n", found, idx[0], idx[1], idx[2], idx[3]);
            // if (found == 0) CRIT_HLT();
        } else {
            table[*i].whole = PTE_RESERVED; // Do the same shit as linux
            // Basically says the page is reserved, and if accessed but is not actually mapped it raises
            // the page fault interrupt and then map the page via the interrupt
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
    if (table[idx[pageType]].sreserved || table[idx[pageType]].present) return 0;
    table[idx[pageType]].whole = MAKE_PAGE_ENTRY(addr, flags);
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
