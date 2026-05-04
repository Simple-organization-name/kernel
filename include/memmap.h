#ifndef __KMAP_H__
#define __KMAP_H__

#include <memTables.h>
#include <kmemory.h>

#define _IDX_TO_ARR(idx) {}

typedef enum _PageType {
    PTE_PML4 = 0,
    PTE_PDP,    // PageDirectoryPointer:    1GiB
    PTE_PD,     // PageDirectory:           2MiB
    PTE_PT,     // PageTable:               4kiB
} PageType;

void memmap_clearPage(PhysAddr addr);

PhysAddr memmap_getMapping(VirtAddr virtual, uint8_t *pageLevel);

int memmap_findSlot(uint8_t targetType, uint16_t *idx);
size_t memmap_findRange(uint8_t targetType, uint16_t *idx, size_t count);

int memmap_map(const uint16_t *idx, uint8_t pageType, PhysAddr addr, uint64_t flags);
int memmap_unmap(VirtAddr virt, PhysAddr *phys);

int memmap_reserve(VirtAddr addr, PageType pageType);
int memmap_sreserve(VirtAddr addr, PageType pageType);
int memmap_unReserve(VirtAddr addr);
int memmap_unSReserve(VirtAddr addr);

#endif
