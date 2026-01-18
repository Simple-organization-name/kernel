#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "asm.h"
#include "attribute.h"
#include "kmemory.h"

__attribute_maybe_unused__
void *memset(void *dest, int val, size_t count) {
    for (size_t i = 0; i < count; i++)
        ((uint8_t *)dest)[i] = val;
    return dest;
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
