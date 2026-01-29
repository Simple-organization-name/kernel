#include "kmemory.h"
#include "asm.h"
#include "memmap.h"
#include "buddy.h"

void clearPageTable(PhysAddr addr) {
    ((PageEntry *)PT(510, 508, 511))[0].whole = MAKE_PAGE_ENTRY(addr, PTE_P | PTE_NX | PTE_RW);
    invlpg((uint64_t)VA(510, 508, 511, 0));
    memset(VA(510, 508, 511, 0), 0, 4096);
    ((PageEntry *)PT(510, 508, 511))[0].whole = 0;
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
        if (curType == targetType && !table[i].present) { // if it is the right level and the slot is free
            if (curType == targetType)
                return 1;

        } else if (curType != targetType && table[i].present && !table[i].pageSize) {
            if (_findEmptySlotPageIdx(targetType, idx, curType + 1))
                return 1;
        }
    }

    idx[curType] = 0;
    return 0;
}

inline int findEmptySlotPageIdx(uint8_t targetType, uint16_t *idx) {
    return _findEmptySlotPageIdx(targetType, idx, 0);
}

inline void mapPage(uint16_t *idx, uint8_t pageType, PhysAddr addr, uint64_t flags) {
    PageEntry *table = getTable(pageType, idx);
    ((PageEntry *)table)[idx[pageType]].whole = MAKE_PAGE_ENTRY(addr, flags);
    invlpg((uint64_t)VA_ARRAY(idx));
}

int unmapPage(VirtAddr virtual) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    PageEntry *entry = ((PageEntry *)PML4()) + pml4_index;
    if (!entry->present) return 1;

    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = ((PageEntry *)PDPT(pml4_index)) + pdpt_index;
    if (!entry->present) return 1;
    if (entry->pageSize) {
        entry->whole = 0;
        invlpg(virtual);
        return 0;
    }

    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = ((PageEntry *)PD(pml4_index, pdpt_index)) + pd_index;
    if (!entry->present) return 1;
    if (entry->pageSize) {
        entry->whole = 0;
        invlpg(virtual);
        return 0;
    }

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = ((PageEntry *)PT(pml4_index, pdpt_index, pd_index)) + pt_index;
    if (!entry->present) return 1;
    entry->whole = 0;
    invlpg(virtual);
    return 0;
}

PhysAddr getMapping(VirtAddr virtual, uint8_t *pageLevel) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    PageEntry entry = ((PageEntry *)PML4())[pml4_index];
    if (!entry.present) return -1;

    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = ((PageEntry *)PDPT(pml4_index))[pdpt_index];
    if (!entry.present) return -1;
    if (entry.pageSize) {
        if (pageLevel) *pageLevel = PTE_PDP;
        return entry.whole & PTE_ADDR;
    }

    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = ((PageEntry *)PD(pml4_index, pdpt_index))[pd_index];
    if (!entry.present) return -1;
    if (entry.pageSize) {
        if (pageLevel) *pageLevel = PTE_PD;
        return entry.whole & PTE_ADDR;
    }

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = ((PageEntry *)PT(pml4_index, pdpt_index, pd_index))[pt_index];
    if (!entry.present) return -1;
    if (pageLevel) *pageLevel = PTE_PT;
    return entry.whole & PTE_ADDR; // pt is always 4KiB so it doesn't have the pageSize flag
}
