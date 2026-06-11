#include <lock.h>
#include <kmemory.h>
#include <asm.h>
#include <buddy.h>
#include <kerror.h>
#include <memmap.h>

static kernel_lock  _memmapLock = LOCK_INIT;

static PageEntry    *_getTable(const PageType type, const uint16_t * const idx);
static int          _memmap_map(const uint16_t *idx, PageType pageType, PhysAddr addr, uint64_t flags);
static int          _memmap_unmap(VirtAddr virt, PhysAddr *phys);
static void         _memmap_clearPage(const PhysAddr addr);
static int          _findEmptySlot(const PageType targetType, uint16_t * const idx, const PageType curType);
static size_t       _findEmptyRange(const PageType targetType, uint16_t * const idx, const size_t count, const PageType curType, uint16_t * const curIdx, size_t found);
static size_t       _createNeededTable(const PageType targetType, uint16_t * const curIdx, const size_t count, const PageType curType);
static PhysAddr     _memmap_getMapping(VirtAddr virtual, PageType *pageLevel);
static int          _memmap_reserve(VirtAddr addr, PageType pageType);
static int          _memmap_sreserve(VirtAddr addr, PageType pageType);
static int          _memmap_unReserve(VirtAddr addr);
static int          _memmap_unSReserve(VirtAddr addr);


static PageEntry *_getTable(const PageType type, const uint16_t * const idx) {
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

static int _memmap_map(const uint16_t *idx, PageType pageType, PhysAddr addr, uint64_t flags) {
    PageEntry *table = _getTable(pageType, idx);
    PageEntry *entry = &table[idx[pageType]];
    if (entry->present) {
        PRINT_WARN("Page used: P: %d, abort mapping\n", entry->present);
        return 0;
    }
    entry->whole = MAKE_PAGE_ENTRY(addr, flags);
    invlpg((uint64_t)VA_ARRAY(idx));
    return 1;
}

// return 0 if failed, 2 if unmapped a mapped physical page,
// 1 if the address was only reserved but not mapped to an actual physical page
static int _memmap_unmap(VirtAddr virt, PhysAddr *phys) {
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
    if (phys) *phys = entry->dest;
    bool wasPresent = entry->present;
    entry->whole = 0;
    invlpg(virt);
    return 1 + wasPresent;
}

static void _memmap_clearPage(const PhysAddr addr) {
    static const uint16_t idx[] = {510, 508, 511, 0};
    static const uint64_t virt = (uint64_t)VA_ARRAY(idx);
    _memmap_map(idx, PTE_PT, addr, PTE_NX | PTE_RW);
    memset((void *)virt, 0, 4096);
    _memmap_unmap(virt, NULL);
}

static int _findEmptySlot(const PageType targetType, uint16_t * const idx, const PageType curType) {
    PageEntry * const table = _getTable(curType, idx);

    for (uint16_t i = idx[curType]; i < (curType == PTE_PML4 ? 511 : 512); i++) {
        // kprintf("targetType: %u, curType: %u, curIdx: %u\n", targetType, curType, i);
        idx[curType] = i;
        if (curType == targetType && !table[i].sreserved) { // if it is the right level and the slot is free
            table[i].whole = PTE_R;
            return 1;
        }

        else if (curType != targetType && !table[i].pageSize) {
            if (!table[i].sreserved) {
                PhysAddr page = buddy_alloc(BUDDY_4K);
                if (!_memmap_map(idx, curType, page, PTE_RW)) {
                    PRINT_ERR("Failed to map page\n");
                    CRIT_HLT();
                }
            }
            if (_findEmptySlot(targetType, idx, curType + 1))
                return 1;
        }
    }

    idx[curType] = 0;
    return 0;
}

static size_t _findEmptyRange(const PageType targetType, uint16_t * const idx, const size_t count, const PageType curType, uint16_t * const curIdx, size_t found) {
    // Get the page table associated with the current type at the current index in the paging
    PageEntry * const table = _getTable(curType, curIdx);

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
                found += 512 * (1UL<<(9 * (curType - targetType)));
            } else {
                found = _findEmptyRange(targetType, idx, count, curType + 1, curIdx, found); // Recurse in the next level
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
static size_t _createNeededTable(const PageType targetType, uint16_t * const curIdx, const size_t count, const PageType curType) {
    PRINT_DEBUG("Called with targetType: %u, curIdx: {%u %u %u %u}, count: %U, curType: %u\n", targetType, curIdx[0], curIdx[1], curIdx[2], curIdx[3], count, curType);
    PageEntry * const table = _getTable(curType, curIdx);
    const uint16_t maxIdx = curType == PTE_PML4 ? 511 : 512;
    size_t reserved = 0;
    for (uint16_t *i = &curIdx[curType]; *i < maxIdx; ++*i) {
        if (curType == targetType) {
            // PRINT_DEBUG("i: %u, &table[*i]: %X\n", *i, &table[*i]);
            if (table[*i].present) return 0; // Memory map changed, range is not valid anymore
            table[*i].reserved = 1;
            reserved++;
            // PRINT_DEBUG("Reserved %U pages\n", reserved);
        } else {
            if (!table[*i].present) {
                const PhysAddr phys = buddy_alloc(BUDDY_4K);
                _memmap_clearPage(phys);
                if (!_memmap_map(curIdx, curType, phys, PTE_RW)) {
                    PRINT_WARN("Failed to map new page\n"); // Memory map changed, range is not valid anymore
                    return 0;
                }
                // invlpg((uint64_t)VA_ARRAY(curIdx));
                PRINT_DEBUG("Mapped new table at idx: %u %u %u %u, pa: %p, va: %p, type: %u\n", curIdx[0], curIdx[1], curIdx[2], curIdx[3], phys, VA_ARRAY(curIdx), curType);
            }
            PRINT_DEBUG("Recursive call\n");
            reserved += _createNeededTable(targetType, curIdx, count - reserved, curType + 1);
            PRINT_DEBUG("End of recursive call\n");
            PRINT_DEBUG("Reserved %U pages\n", reserved);
            curIdx[curType + 1] = 0;
        }

        if (reserved == count) return reserved;
    }

    return reserved;
}

static PhysAddr _memmap_getMapping(VirtAddr virtual, PageType *pageLevel) {
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

static int _memmap_reserve(VirtAddr addr, PageType pageType)
{
    uint16_t idx[4] = {
        (addr >> 39) & 0x1FF,
        (addr >> 30) & 0x1FF,
        (addr >> 21) & 0x1FF,
        (addr >> 12) & 0x1FF
    };
    PageEntry *table = _getTable(pageType, idx);
    if (!PAGE_TABLE_SLOT_AVAILABLE(table[idx[pageType]])) return 0;
    table[idx[pageType]].whole = PTE_R;
    return 1;
}

static int _memmap_sreserve(VirtAddr addr, PageType pageType)
{
    uint16_t idx[4] = {
        (addr >> 39) & 0x1FF,
        (addr >> 30) & 0x1FF,
        (addr >> 21) & 0x1FF,
        (addr >> 12) & 0x1FF
    };
    PageEntry *table = _getTable(pageType, idx);
    if (!PAGE_TABLE_SLOT_AVAILABLE(table[idx[pageType]])) return 0;
    table[idx[pageType]].whole = PTE_SR;
    return 1;
}

static int _memmap_unReserve(VirtAddr addr)
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

static int _memmap_unSReserve(VirtAddr addr)
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

const char *_pageTypeToStr(PageType type) {
    switch (type) {
        case PTE_PML4: return "PML4";
        case PTE_PDP: return "PDPT";
        case PTE_PD: return "PD";
        case PTE_PT: return "PT";
    }
    return "";
}

static void _printMapping(uint16_t *idx, PageType type) {
    PageEntry *table = _getTable(type, idx);
    for (idx[type] = 0; idx[type] < 512; idx[type]++) {
        PageEntry *entry = &table[idx[type]];
        if (!entry->present) continue;
        for (uint8_t i = 0; i < type * 4; i++) kputc(' ');
        kprintf(
            "[%s (%u)] idx: {%u, %u, %u, %u} | RW: %u | PS: %u | G: %u | SR/R: %u/%u | NX: %u | va: %p, pa: %p\n",
            _pageTypeToStr(type), type, idx[0], idx[1], idx[2], idx[3],
            entry->rw, entry->pageSize, entry->global, entry->sreserved, entry->reserved, entry->xd,
            VA_ARRAY(idx), entry->dest << 12
        );
        if (type != PTE_PT && !entry->pageSize) {
            _printMapping(idx, type + 1);
            for (PageType i = type + 1; i <= PTE_PT; i++)
                idx[i] = 0;
        }
    }
}

void memmap_printMapping() {
    kprintf("  ---==== Mapping ====---\n");
    uint16_t idx[4] = {0};
    _printMapping(idx, 0);
    kprintf("  ---=================---\n\n");
}

void memmap_clearPage(const PhysAddr addr) {
    LOCK_SPINLOCK(&_memmapLock);
    _memmap_clearPage(addr);
    LOCK_RELEASE(&_memmapLock);
}

/**
 * Get an empty slot starting at indexes in idx
 * \param targetType The page type needed
 * \param idx An array of 4 `uint16_t` to store the indexes of the empty slot if found any
 * \return 1 if slot found, 0 else
 */
int memmap_findSlot(PageType targetType, uint16_t * const idx) {
    LOCK_SPINLOCK(&_memmapLock);
    int found = _findEmptySlot(targetType, idx, PTE_PML4);
    LOCK_RELEASE(&_memmapLock);
    return found;
}

/**
 * \param targetType The type of page needed
 * \param idx The array of 4 uint16_t (`uint16_t[4]`) where to store the start of the range if found
 * \param count The number of pages needed
 * \return The number of pages if successful, 0 else
 */
size_t memmap_findRange(const PageType targetType, uint16_t *idx, const size_t count) {
    uint16_t curIdx[4];
    memcpy(curIdx, idx, sizeof(uint16_t) * 4);
    PRINT_DEBUG("idx: %u %u %u %u, curIdx: %u %u %u %u\n", idx[0], idx[1], idx[2], idx[3], curIdx[0], curIdx[1], curIdx[2], curIdx[3]);

    LOCK_SPINLOCK(&_memmapLock);
    const size_t found = _findEmptyRange(targetType, idx, count, PTE_PML4, curIdx, 0);
    // PRINT_WARN("found: %U, at: %u %u %u %u\n", found, curIdx[0], curIdx[1], curIdx[2], curIdx[3]);
    if (found < count) {
        LOCK_RELEASE(&_memmapLock);
        return 0;
    }
    PRINT_DEBUG("idx: %u %u %u %u, curIdx: %u %u %u %u\n", idx[0], idx[1], idx[2], idx[3], curIdx[0], curIdx[1], curIdx[2], curIdx[3]);

    memcpy(curIdx, idx, sizeof(uint16_t) * 4);
    const size_t mappedPages = _createNeededTable(targetType, curIdx, count, 0);

    // PRINT_WARN("mapped pages: %U\n", mappedPages);
    if (mappedPages != count) {
        // TODO: NEED TO CLEAR THE RESERVED FLAGS OF THE ALREADY MAPPED PAGES
        LOCK_RELEASE(&_memmapLock);
        return 0;
    }
    LOCK_RELEASE(&_memmapLock);

    PRINT_DEBUG("idx: %u %u %u %u, curIdx: %u %u %u %u\n", idx[0], idx[1], idx[2], idx[3], curIdx[0], curIdx[1], curIdx[2], curIdx[3]);
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
int memmap_map(const uint16_t *idx, PageType pageType, PhysAddr addr, uint64_t flags) {
    LOCK_SPINLOCK(&_memmapLock);
    int status = _memmap_map(idx, pageType, addr, flags);
    LOCK_RELEASE(&_memmapLock);
    return status;
}

int memmap_unmap(VirtAddr virt, PhysAddr *phys) {
    LOCK_SPINLOCK(&_memmapLock);
    int status = _memmap_unmap(virt, phys);
    LOCK_RELEASE(&_memmapLock);
    return status;
}

int memmap_reserve(VirtAddr addr, PageType pageType) {
    LOCK_SPINLOCK(&_memmapLock);
    int status = _memmap_reserve(addr, pageType);
    LOCK_RELEASE(&_memmapLock);
    return status;
}

int memmap_sreserve(VirtAddr addr, PageType pageType) {
    LOCK_SPINLOCK(&_memmapLock);
    int status = _memmap_sreserve(addr, pageType);
    LOCK_RELEASE(&_memmapLock);
    return status;
}

int memmap_unReserve(VirtAddr addr) {
    LOCK_SPINLOCK(&_memmapLock);
    int status = _memmap_unReserve(addr);
    LOCK_RELEASE(&_memmapLock);
    return status;
}

int memmap_unSReserve(VirtAddr addr) {
    LOCK_SPINLOCK(&_memmapLock);
    int status = _memmap_unSReserve(addr);
    LOCK_RELEASE(&_memmapLock);
    return status;
}

PhysAddr memmap_getMapping(VirtAddr virtual, PageType *pageLevel) {
    LOCK_SPINLOCK(&_memmapLock);
    PhysAddr addr = _memmap_getMapping(virtual, pageLevel);
    LOCK_RELEASE(&_memmapLock);
    return addr;
}
