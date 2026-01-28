#ifndef __KMAP_H__
#define __KMAP_H__

#include "memTables.h"
#include "kmemory.h"
#include "kerror.h"

typedef enum _PageType {
    PTE_PML4,
    PTE_PDP,    // PageDirectoryPointer:    1GiB
    PTE_PD,     // PageDirectory:           2MiB
    PTE_PT,     // PageTable:               4kiB
} PageType;

void clearPageTable(PhysAddr addr);

PhysAddr getMapping(VirtAddr virtual, uint8_t *pageLevel);
/**
 * Get an empty slot starting at indexes in idx
 * \param targetType The page type needed
 * \param idx An array of 4 `uint16_t` to store the indexes of the empty slot if found any
 * \return 1 if slot found, 0 else
 */
int findEmptySlotPageIdx(uint8_t targetType, uint16_t *idx);
int unmapPage(VirtAddr virtual);

#endif
