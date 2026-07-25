#ifndef __KMAP_H__
#define __KMAP_H__

#include <memTables.h>
#include <kmemory.h>

void memmap_printInfo(uint16_t *idx, PageType type);
void memmap_printMapping();

void memmap_clearPage(PhysAddr addr);

PhysAddr memmap_getMapping(VirtAddr virtual, PageType *pageLevel);

int memmap_findSlot(PageType targetType, uint16_t *idx);
size_t memmap_findRange(PageType targetType, uint16_t *idx, size_t count);

int memmap_map(const uint16_t *idx, PageType pageType, PhysAddr addr, uint64_t flags);
int memmap_unmap(VirtAddr virt, PhysAddr *phys);

int memmap_reserve(VirtAddr addr, PageType pageType);
int memmap_sreserve(VirtAddr addr, PageType pageType);
int memmap_unReserve(VirtAddr addr);
int memmap_unSReserve(VirtAddr addr);

#endif
