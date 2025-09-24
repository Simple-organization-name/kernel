#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <asm.h>

#include "memTables.h"
#include "kalloc.h"
#include "kterm.h"

volatile MemMap *physMemoryMap = NULL;

inline static void initMemoryBitmap(volatile MemoryRange *validMemory, uint16_t validMemoryCount) {
    // Set all memory to invalid
    volatile MemBitmap *memBitmap = (MemBitmap *)memoryBitmap_va;
    for (size_t i = 0; i < BMP_SIZE; i++)
        memBitmap->whole[i].value = 255U;

    // For every valid range set the corresponding bit
    for (uint16_t i = 0; i < validMemoryCount; i++) {
        MemoryRange target = validMemory[i];
        uint64_t start = target.start / 4096;
        uint64_t pageCount = target.size / 4096;
        for (uint64_t j = 0; j < pageCount; j++)
            memBitmap->level1[(start + j)/8].value &= ~(1<<(j%8));
    }

    // Cascade level
    for (uint64_t j = 0; j < BMP_L1_SIZE; j++) {
        if (memBitmap->level1[j].value != 255U)
            memBitmap->level2[j/8].value &= ~(1<<(j%8));
    }
    for (uint64_t j = 0; j < BMP_L2_SIZE; j++) {
        if (memBitmap->level2[j].value != 255U)
            memBitmap->level3[j/8].value &= ~(1<<(j%8));
    }
    for (uint64_t j = 0; j < BMP_L3_SIZE; j++) {
        if (memBitmap->level3[j].value != 255U)
            memBitmap->level4[j/8].value &= ~(1<<(j%8));
    }
    for (uint64_t j = 0; j < BMP_L4_SIZE; j++) {
        if (memBitmap->level4[j].value != 255U)
            memBitmap->level5[j/8].value &= ~(1<<(j%8));
    }
    for (uint64_t j = 0; j < BMP_L5_SIZE; j++) {
        if (memBitmap->level5[j].value != 255U)
            memBitmap->level6[j/8].value &= ~(1<<(j%8));
    }
}

void initPhysMem() {
    // keep volatile or it breaks
    volatile MemoryRange validMemory[256] = {0};
    uint16_t validMemoryCount = 0;

    for (uint16_t i = 0; i < physMemoryMap->count; i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemoryMap->map + physMemoryMap->descSize * i);
        switch (desc->Type) {
            case EfiLoaderCode:
            case EfiLoaderData:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiConventionalMemory:
                // kprintf("Memory type: %d | ", desc->Type);
                if (validMemoryCount > 0 &&
                    validMemory[validMemoryCount - 1].start +
                    validMemory[validMemoryCount - 1].size ==
                    desc->PhysicalStart)
                {
                    // kputs("Adjacent memory | ");
                    validMemory[validMemoryCount - 1].size += desc->NumberOfPages * 4096;
                }
                else
                {
                    validMemory[validMemoryCount].start = desc->PhysicalStart;
                    validMemory[validMemoryCount].size = desc->NumberOfPages * 4096;
                    validMemoryCount++;
                }
                // kprintf("Memory start: %X, memory size: %X\n", validMemory[validMemoryCount - 1].start, validMemory[validMemoryCount -1].size);
                break;
            default:
                break;
        }
    }

    #define align(addr) (((uint64_t)(addr) + 0x1FFFFF) & ~0x1FFFFF)
    int16_t where = -1;
    uint64_t totalSize = 0;
    physAddr bitmapBase = 0;
    for (uint16_t i = 0; i < validMemoryCount; i++)
    {
        bitmapBase = align(validMemory[i].start);
        totalSize = bitmapBase - validMemory[i].start + (2<<20);
        if (validMemory[i].size >= totalSize) {
            where = i;
            break;
        }
    }
    #undef align

    if (where == -1) {
        kprintf("Could not find enough contiguous memory for memory bitmap\n");
        cli();
        while (1) hlt();
    }

    if (bitmapBase != validMemory[where].start) {
        // Shift all MemoryRange
        for (uint16_t i = validMemoryCount - 1; i >= where; i--) {
            MemoryRange tmp = validMemory[i];
            validMemory[i + 1].size = tmp.size;
            validMemory[i + 1].start = tmp.start;
        }
        
        // Segment target in 2 parts
        // First part is the part before the MemBitmap
        validMemory[where].size = bitmapBase - validMemory[where].start;
        where++;
        // Second part (right below) is after the MemBitmap
    }
    validMemory[where].start = bitmapBase + (2<<20);
    validMemory[where].size -= totalSize;

    kprintf("Bitmap base: 0x%X\n", bitmapBase);

    kputs("\nValid memory after memory bitmap allocation:\n");
    for (uint16_t i = 0; i < validMemoryCount; i++)
        kprintf("Memory start: %X, memory size: %X\n", validMemory[i].start, validMemory[i].size);
    kputc('\n');

    ((pte_t *)PD(510, 510))[511].whole = ((uintptr_t)bitmapBase & PTE_ADDR) | PTE_P | PTE_RW | PTE_PS | PTE_NX;
    invlpg(memoryBitmap_va);

    initMemoryBitmap(validMemory, validMemoryCount);

    volatile pte_t *pageTableEntry = (pte_t *)MEM_BMP_PAGE_TABLE_START(memoryBitmap_va);
    uint64_t i = 0;
    for (; i < MEM_BMP_PAGE_TABLE_SIZE(memoryBitmap_va); i++)
        CLEAR_PT(pageTableEntry + i);
    kprintf("Nb pte free in the 2mb mem bmp: %U\n", i);
}

physAddr resPhysMemory(size_t size) {
    if (size > BMP_L6_MEM_SIZE) return -1;
    MemBitmap *bitmap = (MemBitmap *)memoryBitmap_va;
    uint64_t lvl6Idx, lvl5Idx, lvl4Idx, lvl3Idx, lvl2Idx, lvl1Idx;

    for (lvl6Idx = 0; lvl6Idx < BMP_L6_SIZE * 8; lvl6Idx++) {
        if (!(bitmap->level6[lvl6Idx/8].value & (1<<(lvl6Idx%8)))) {
            if (size > BMP_L5_MEM_SIZE) return lvl6Idx * BMP_L6_MEM_SIZE;
            break;
        }
    }
    for (lvl5Idx = lvl6Idx * BMP_L6_MEM_SIZE; lvl5Idx < lvl6Idx * BMP_L6_MEM_SIZE + 8; lvl5Idx++) {
        if (!(bitmap->level5[lvl5Idx/8].value & (1<<(lvl5Idx%8)))) {
            if (size > BMP_L4_MEM_SIZE) return lvl5Idx * BMP_L5_MEM_SIZE;
            break;
        }
    }
    for (lvl4Idx = lvl5Idx * BMP_L5_MEM_SIZE; lvl4Idx < lvl5Idx * BMP_L5_MEM_SIZE + 8; lvl4Idx++) {
        if (!(bitmap->level4[lvl4Idx/8].value & (1<<(lvl4Idx%8)))) {
            if (size > BMP_L3_MEM_SIZE) return lvl4Idx * BMP_L4_MEM_SIZE;
            break;
        }
    }
    for (lvl3Idx = lvl4Idx * BMP_L4_MEM_SIZE; lvl3Idx < lvl4Idx * BMP_L4_MEM_SIZE + 8; lvl3Idx++) {
        if (!(bitmap->level3[lvl3Idx/8].value & (1<<(lvl3Idx%8)))) {
            if (size > BMP_L2_MEM_SIZE) return lvl3Idx * BMP_L3_MEM_SIZE;
            break;
        }
    }
    for (lvl2Idx = lvl3Idx * BMP_L3_MEM_SIZE; lvl2Idx < lvl3Idx * BMP_L3_MEM_SIZE + 8; lvl2Idx++) {
        if (!(bitmap->level2[lvl2Idx/8].value & (1<<(lvl2Idx%8)))) {
            if (size > BMP_L1_MEM_SIZE) return lvl2Idx * BMP_L2_MEM_SIZE;
            break;
        }
    }
    for (lvl1Idx = lvl2Idx * BMP_L2_MEM_SIZE; lvl1Idx < lvl2Idx * BMP_L2_MEM_SIZE + 8; lvl1Idx++) {
        if (!(bitmap->level1[lvl1Idx/8].value & (1<<(lvl1Idx%8)))) {
            return lvl1Idx * BMP_L1_MEM_SIZE;
        }
    }

    return -1;
}

void printMemBitmap() {
    MemBitmap *bitmap = (MemBitmap *)(memoryBitmap_va);
    uint64_t count = 0;
    uint8_t sucBit = bitmap->level1[0].bit1;
    for (uint64_t i = 0; i < BMP_L1_SIZE; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            register uint8_t bit = bitmap->level1[i].value & (1 << j);
            bit >>= j;
            if (bit != sucBit) {
                kprintf("%Dx%s ", count, sucBit ? "taken" : "free");
                count = 1;
                sucBit = bit;
            } else count++;
        }
    }
    knewline();
    count = 0;
    sucBit = bitmap->level1[0].bit1;
    for (uint64_t i = 0; i < BMP_L2_SIZE; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            register uint8_t bit = bitmap->level2[i].value & (1 << j);
            bit >>= j;
            if (bit != sucBit) {
                kprintf("%Dx%s ", count, sucBit ? "taken" : "free");
                count = 1;
                sucBit = bit;
            } else count++;
        }
    }
    knewline();
    count = 0;
    sucBit = bitmap->level1[0].bit1;
    for (uint64_t i = 0; i < BMP_L3_SIZE; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            register uint8_t bit = bitmap->level3[i].value & (1 << j);
            bit >>= j;
            if (bit != sucBit) {
                kprintf("%Dx%s ", count, sucBit ? "taken" : "free");
                count = 1;
                sucBit = bit;
            } else count++;
        }
    }
    knewline();
    count = 0;
    sucBit = bitmap->level1[0].bit1;
    for (uint64_t i = 0; i < BMP_L4_SIZE; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            register uint8_t bit = bitmap->level4[i].value & (1 << j);
            bit >>= j;
            if (bit != sucBit) {
                kprintf("%Dx%s ", count, sucBit ? "taken" : "free");
                count = 1;
                sucBit = bit;
            } else count++;
        }
    }
    knewline();
    count = 0;
    sucBit = bitmap->level1[0].bit1;
    for (uint64_t i = 0; i < BMP_L5_SIZE; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            register uint8_t bit = bitmap->level5[i].value & (1 << j);
            bit >>= j;
            if (bit != sucBit) {
                kprintf("%Dx%s ", count, sucBit ? "taken" : "free");
                count = 1;
                sucBit = bit;
            } else count++;
        }
    }
    knewline();
    count = 0;
    sucBit = bitmap->level1[0].bit1;
    for (uint64_t i = 0; i < BMP_L6_SIZE; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            register uint8_t bit = bitmap->level6[i].value & (1 << j);
            bit >>= j;
            if (bit != sucBit) {
                kprintf("%Dx%s ", count, sucBit ? "taken" : "free");
                count = 1;
                sucBit = bit;
            } else count++;
        }
    }
    knewline();
}


#define map(physical) entry->whole = (uint64_t)((uintptr_t)physical & PTE_ADDR) | PTE_P | PTE_RW
bool mapPage(physAddr physical, virtAddr virtual) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    pte_t *entry = ((pte_t *)PML4()) + pml4_index;
    if (!entry->present) return 0;

    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = ((pte_t *)PDPT(pml4_index)) + pdpt_index;
    if (!entry->present) return 0;
    if (entry->pageSize) return 0;

    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = ((pte_t *)PD(pml4_index, pdpt_index)) + pd_index;
    if (!entry->present) return 0;
    if (entry->pageSize) return 0;

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = ((pte_t *)PT(pml4_index, pdpt_index, pd_index)) + pt_index;
    if (entry->present) return 0;
    map(physical);
    return 1;
}

bool unmapPage(virtAddr virtual) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    pte_t *entry = ((pte_t *)PML4()) + pml4_index;
    if (!entry->present) return 0;
    
    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = ((pte_t *)PDPT(pml4_index)) + pdpt_index;
    if (!entry->present) return 0;
    if (entry->pageSize) {
        entry->whole = 0;
        invlpg(virtual);
        return 1;
    }
    
    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = ((pte_t *)PD(pml4_index, pdpt_index)) + pd_index;
    if (!entry->present) return 0;
    if (entry->pageSize) {
        entry->whole = 0;
        invlpg(virtual);
        return 1;
    }

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = ((pte_t *)PT(pml4_index, pdpt_index, pd_index)) + pt_index;
    if (!entry->present) return 0;
    entry->whole = 0;
    invlpg(virtual);
    return 1;
}

physAddr getMapping(virtAddr virtual, uint8_t *pageLevel) {
    uint16_t pml4_index = (virtual >> 39) & 0x1FF;
    pte_t entry = ((pte_t *)PML4())[pml4_index];
    if (!entry.present) return 0;

    uint16_t pdpt_index = (virtual >> 30) & 0x1FF;
    entry = ((pte_t *)PDPT(pml4_index))[pdpt_index];
    if (!entry.present) return 0;
    if (entry.pageSize) {
        if (pageLevel) *pageLevel = 3;
        return entry.whole & PTE_ADDR;
    }

    uint16_t pd_index = (virtual >> 21) & 0x1FF;
    entry = ((pte_t *)PD(pml4_index, pdpt_index))[pd_index];
    if (!entry.present) return 0;
    if (entry.pageSize) {
        if (pageLevel) *pageLevel = 2;
        return entry.whole & PTE_ADDR;
    }

    uint16_t pt_index = (virtual >> 12) & 0x1FF;
    entry = ((pte_t *)PT(pml4_index, pdpt_index, pd_index))[pt_index];
    if (!entry.present) return 0;
    if (pageLevel) *pageLevel = 1;
    return entry.whole & PTE_ADDR; // pt is always 4KiB so it doesn't have the pageSize flag
}
