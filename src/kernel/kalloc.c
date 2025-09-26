#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <asm.h>

#include "memTables.h"
#include "kalloc.h"
#include "kterm.h"

volatile MemMap *physMemoryMap = NULL;

inline uint64_t bmpGetOffset(uint8_t level) {
    if (level > 5) return 0;
    uint64_t offset = 0;
    for (uint8_t i = 0; i < level; i++)
        offset += BMP_SIZE_OF(i);
    return offset;
}

inline static void initMemoryBitmap(volatile MemoryRange *validMemory, uint16_t validMemoryCount) {
    // Set all memory to invalid
    MemBitmap *memBitmap = (MemBitmap *)memoryBitmap_va;
    for (size_t i = 0; i < BMP_SIZE; i++)
        memBitmap->whole[i].value = 255U;

    // For every valid range set the corresponding bit
    for (uint16_t i = 0; i < validMemoryCount; i++) {
        MemoryRange target = validMemory[i];
        uint64_t start = target.start / 4096;
        uint64_t pageCount = target.size / 4096;
        for (uint64_t j = 0; j < pageCount; j++)
            memBitmap->level0[(start + j)/8].value &= ~(1<<(j%8));
    }

    // Cascade level
    for (uint64_t i = 0; i < 5; i++) {
        for (uint64_t j = 0; j < BMP_SIZE_OF(i); j++) {
            if (memBitmap->whole[bmpGetOffset(i) + j].value != 255U)
                memBitmap->whole[bmpGetOffset(i+1) + j/8].value &= ~(1<<(j%8));
        }
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

    // If there's memory available before the start of the memory mapping made by the alignment
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

    // Clear page tables available after memory bitmap
    volatile pte_t *pageTableEntry = (pte_t *)MEM_BMP_PAGE_TABLE_START(memoryBitmap_va);
    uint64_t i = 0;
    for (; i < MEM_BMP_PAGE_TABLE_SIZE(memoryBitmap_va); i++)
        CLEAR_PT(pageTableEntry + i);
    kprintf("Nb pte free in the 2mb mem bmp: %U\n\n", i);
}

void printMemBitmapLevel(uint8_t n) {
    MemBitmap *bitmap = (MemBitmap *)(memoryBitmap_va);
    uint64_t count = 0;
    uint8_t sucBit = bitmap->whole[bmpGetOffset(n)].bit1;
    kprintf("Level %d memory bitmap (memSize: %X, size: %U):\n", n, BMP_MEM_SIZE_OF(n), BMP_SIZE_OF(n));
    for (uint64_t i = 0; i < BMP_SIZE_OF(n); i++) {
        for (uint8_t j = 0; j < 8; j++) {
            register uint8_t bit = bitmap->whole[bmpGetOffset(n) + i].value & (1 << j);
            bit >>= j;
            if (bit != sucBit) {
                kprintf("%Dx%c ", count, sucBit ? 'O' : 'F');
                count = 1;
                sucBit = bit;
            } else count++;
        }
    }
    kprintf("%Dx%c\n", count, sucBit ? 'O' : 'F');
}

void printMemBitmap() {
    for (uint8_t n = 0; n < 6; n++) {
        printMemBitmapLevel(n);
        knewline();
    }
}

inline void rippleBitFlip(bool targetState, uint8_t level, volatile uint64_t idx[6]) {
    if (level > 5) return;
    MemBitmap *bitmap = (MemBitmap *)memoryBitmap_va;

    // kprintf("level: %u\n", level);
    for (uint8_t i = level; i < 5; i++) {
        // kprintf("i: %u, idx: %U, value: %u\n", i, idx[i]/BMP_JUMP, bitmap->whole[bmpGetOffset(i) + idx[i]/BMP_JUMP].value);
        bool filled = (bitmap->whole[bmpGetOffset(i) + idx[i]/BMP_JUMP].value == UINT8_MAX);
        if (targetState ? filled : !filled) {
            uint8_t targetBit = idx[i+1]%BMP_JUMP;
            bitmap->whole[bmpGetOffset(i+1) + idx[i+1]/BMP_JUMP].value ^= 1<<(targetBit);
        } else return;
    }
}

inline bool checkMem(uint8_t curLevel, uint64_t index) {
    MemBitmap *bitmap = (MemBitmap *)memoryBitmap_va;
    for (uint8_t i = curLevel - 1; i < 5; i--) {
        uint64_t levelOffset = bmpGetOffset(i);
        uint64_t thingsToCheck = (1<<(BMP_JUMP_POW2 * (curLevel-i)));
        kprintf("%U", thingsToCheck);
        for (uint64_t uwu = 0; uwu < thingsToCheck; uwu++) {
            if (bitmap->whole[levelOffset + index * BMP_SIZE_OF(i) / BMP_SIZE_OF(curLevel) + uwu].value != 0) {
                kputc('!');
                return false;
            }
        }
    }
    return true;
}

physAddr _resPhysMemory(size_t size, uint8_t curLevel, volatile uint64_t idx[6]) {
    if (size > BMP_MEM_SIZE_OF(5)) return -1;
    MemBitmap *bitmap = (MemBitmap *)memoryBitmap_va;
    // kprintf("%d %d %d %d %d %d\n", idx[0], idx[1], idx[2], idx[3], idx[4], idx[5]);

    uint64_t i = curLevel != 5 ? idx[curLevel + 1] * 8 : 0;
    for (; i < BMP_SIZE_OF(curLevel) * BMP_JUMP; i++) {
        // kprintf("level: %d i: %U, idx: %U, ", curLevel, i, bmpGetOffset(curLevel) + i/BMP_JUMP);
        // kprintf("current bit: %d\n", bitmap->whole[bmpGetOffset(curLevel) + i/BMP_JUMP].value & (1<<(i%BMP_JUMP)));
        if (!(bitmap->whole[bmpGetOffset(curLevel) + i/BMP_JUMP].value & (1<<(i%BMP_JUMP)))) {
            idx[curLevel] = i;
            physAddr addrOffset = i * BMP_MEM_SIZE_OF(curLevel);
            if (size > BMP_MEM_SIZE_OF(curLevel-1)) {
                if (curLevel == 0 || checkMem(curLevel, idx[curLevel])) {
                    bitmap->whole[bmpGetOffset(curLevel) + i/BMP_JUMP].value |= (1<<(i%BMP_JUMP));
                    rippleBitFlip(1, curLevel, idx);
                    return addrOffset;
                }
            } else {
                physAddr nextLevelOffset = _resPhysMemory(size, curLevel - 1, idx);
                if (nextLevelOffset != -1UL) {
                    // kprintf("|%d %d %d %d %d %d|", idx[0], idx[1], idx[2], idx[3], idx[4], idx[5]);
                    return nextLevelOffset;
                }
            }
        }
    }
    return -1;
}

physAddr resPhysMemory(uint8_t level, uint64_t count) {
    volatile uint64_t idx[6];
    for (uint8_t i = 0; i < 6; i++) idx[i] = 0;
    return _resPhysMemory(BMP_MEM_SIZE_OF(level) * count, 5, idx);
}

virtAddr allocVirtMemory(uint8_t level, uint64_t count)
{
    physAddr memory = resPhysMemory(level, count);
    
    // no risk of memory leak here dw

    return NULL;
}

#define map(entry, physical) entry->whole = (uint64_t)((uintptr_t)physical & PTE_ADDR) | PTE_P | PTE_RW
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
    map(entry, physical);
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
