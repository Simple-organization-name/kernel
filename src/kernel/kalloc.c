#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "asm.h"
#include "attribute.h"
#include "memTables.h"
#include "kalloc.h"
#include "kterm.h"

__attribute_maybe_unused__
void *memset(void *dest, int val, size_t count) {
    for (size_t i = 0; i < count; i++)
        ((uint8_t *)dest)[i] = val;
    return dest;
}

static uint64_t bmpGetOffset[6] = {
    0,
    BMP_SIZE_OF(0),
    BMP_SIZE_OF(0) + BMP_SIZE_OF(1),
    BMP_SIZE_OF(0) + BMP_SIZE_OF(1) + BMP_SIZE_OF(2),
    BMP_SIZE_OF(0) + BMP_SIZE_OF(1) + BMP_SIZE_OF(2) + BMP_SIZE_OF(3),
    BMP_SIZE_OF(0) + BMP_SIZE_OF(1) + BMP_SIZE_OF(2) + BMP_SIZE_OF(3) + BMP_SIZE_OF(4)
};

// inline uint64_t bmpGetOffset(uint8_t level) {
//     if (level > 5) kprintf("Invalid level\n");
//     uint64_t offset = 0;
//     for (uint8_t i = 0; i < level; i++)
//         offset += BMP_SIZE_OF(i);
//     return offset;
// }

// inline static void bmpCacheOffset() {
//     for (uint8_t level = 0; level < 6; level++) {
//         uint64_t offset = 0;
//         for (uint8_t i = 0; i < level; i++)
//             offset += BMP_SIZE_OF(i);
//         bmpGetOffset[level] = offset;
//     }
// }

inline static uint8_t getValidMemRanges(EfiMemMap *physMemoryMap, MemoryRange *validMemory) {
    uint8_t validMemoryCount = 0;

    for (uint8_t i = 0; i < physMemoryMap->count; i++) {
        MemoryDescriptor *desc = (MemoryDescriptor *)((char *)physMemoryMap->map + physMemoryMap->descSize * i);
        switch (desc->Type) {
            case EfiLoaderCode:
            case EfiLoaderData:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiConventionalMemory:
                if (validMemoryCount > 0 &&
                    validMemory[validMemoryCount - 1].start +
                    validMemory[validMemoryCount - 1].size ==
                    desc->PhysicalStart)
                {
                    validMemory[validMemoryCount - 1].size += desc->NumberOfPages * 4096;
                }
                else
                {
                    validMemory[validMemoryCount].start = desc->PhysicalStart;
                    validMemory[validMemoryCount].size = desc->NumberOfPages * 4096;
                    validMemoryCount++;
                }
                break;
            default:
                break;
        }
    }

    return validMemoryCount;
}

inline static void initMemoryBitmap(MemoryRange *validMemory, uint16_t validMemoryCount) {
    // Set all memory to invalid
    MemBitmap *memBitmap = (MemBitmap *)VA_MEM_BMP;
    for (size_t i = 0; i < BMP_SIZE; i++) memBitmap->whole[i].value = 255U;

    // For every valid range set the corresponding bit
    for (uint16_t i = 0; i < validMemoryCount; i++) {
        uint64_t start = validMemory[i].start / 4096;
        uint64_t pageCount = validMemory[i].size / 4096;
        for (uint64_t j = 0; j < pageCount; j++)
            memBitmap->level0[(start + j) / 8].value &= ~(1 << (j % 8));
    }

    // Cascade level
    for (uint64_t i = 0; i < 5; i++) {
        for (uint64_t j = 0; j < BMP_SIZE_OF(i); j++) {
            if (memBitmap->whole[bmpGetOffset[(i)] + j].value != 255U)
                memBitmap->whole[bmpGetOffset[(i + 1)] + j / 8].value &= ~(1 << (j % 8));
        }
    }
}

__attribute_no_vectorize__
void initPhysMem(EfiMemMap *physMemMap) {
    MemoryRange validMemory[256] = {0};
    uint8_t validMemoryCount = getValidMemRanges(physMemMap, validMemory);

    // bmpCacheOffset();

    #define align(addr) (((uint64_t)(addr) + 0x1FFFFF) & ~0x1FFFFF)
    int16_t where = -1;
    uint64_t totalSize = 0;
    PhysAddr bitmapBase = 0;
    for (uint8_t i = 0; i < validMemoryCount; i++) {
        bitmapBase = align(validMemory[i].start);
        totalSize = bitmapBase - validMemory[i].start + (2 << 20);
        if (validMemory[i].size >= totalSize) {
            where = i;
            break;
        }
    }
    #undef align

    if (where == -1) {
        kprintf("Could not find enough contiguous memory for memory bitmap\n");
        CRIT_HLT();
    }

    // If there's memory available before the start of the memory mapping made by the alignment
    if (bitmapBase != validMemory[where].start) {
        // Shift all MemoryRange
        for (uint8_t i = validMemoryCount - 1; i >= where; i--) {
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
    validMemory[where].start = bitmapBase + (2 << 20);
    validMemory[where].size -= totalSize;

    ((PageEntry *)PD(510, 509))[511].whole = (uint64_t)(((uintptr_t)bitmapBase & PTE_ADDR) | PTE_P | PTE_RW | PTE_PS | PTE_NX);
    invlpg(VA_MEM_BMP);

    initMemoryBitmap(validMemory, validMemoryCount);

    PhysAddr physTempPT = (bitmapBase + BMP_SIZE + 0xFFF) & ~0xFFF;
    // kprintf("Phys temp PT: 0x%X\n", physTempPT);
    ((PageEntry *)PD(510, 509))[510].whole = (uint64_t)(((uintptr_t)physTempPT & PTE_ADDR) | PTE_P | PTE_RW | PTE_NX);
    invlpg(VA_TEMP_PT);
}

void printMemBitmapLevel(uint8_t n) {
    if (n > 5) {
        kprintf("Invalid level\n");
        return;
    }
    kprintf("Memory bitmap: level %u\n", n);
    MemBitmap *bitmap = (MemBitmap *)(VA_MEM_BMP);
    uint64_t count = 0;
    uint8_t sucBit = bitmap->whole[bmpGetOffset[(n)]].bit1;
    for (uint64_t i = 0; i < BMP_SIZE_OF(n); i++) {
        for (uint8_t j = 0; j < 8; j++) {
            register uint8_t bit = bitmap->whole[bmpGetOffset[(n)] + i].value & (1 << j);
            bit >>= j;
            if (bit != sucBit) {
                kprintf("%Dx%c ", count, sucBit ? 'O' : 'F');
                count = 1;
                sucBit = bit;
            } else count++;
        }
    }
    knewline();
}

void printMemBitmap() {
    for (uint8_t n = 0; n < 6; n++) {
        printMemBitmapLevel(n);
    }
}

inline static void rippleBitFlip(bool targetState, uint8_t level, uint64_t idx[6]) {
    if (level > 4) return;
    MemBitmap *bitmap = (MemBitmap *)VA_MEM_BMP;

    for (uint8_t i = level; i < 5; i++) {
        bool filled = (bitmap->whole[bmpGetOffset[(i)] + idx[i] / BMP_JUMP].value == UINT8_MAX);
        if (targetState == filled) {
            uint8_t targetBit = idx[i + 1] % BMP_JUMP;
            register uint8_t *byte = &bitmap->whole[bmpGetOffset[(i + 1)] + idx[i + 1] / BMP_JUMP].value;
            *byte = (*byte & ~(1 << targetBit)) | (targetState << targetBit);
        } else return;
    }
}

static bool checkMem(uint8_t curLevel, uint64_t index) {
    MemBitmap *bitmap = (MemBitmap *)VA_MEM_BMP;
    for (uint8_t i = curLevel - 1; i < 5; i--) {
        uint64_t levelOffset = bmpGetOffset[(i)];
        uint64_t thingsToCheck = (1 << (BMP_JUMP_POW2 * (curLevel - i)));
        for (uint64_t uwu = 0; uwu < thingsToCheck; uwu++) {
            if (bitmap->whole[levelOffset + index * BMP_SIZE_OF(i) / BMP_SIZE_OF(curLevel) + uwu].value != 0) {
                return false;
            }
        }
    }
    return true;
}

static PhysAddr _resPhysMemory(uint8_t size, uint64_t count, uint8_t curLevel, uint64_t idx[6]) {
    if (size > 5 || curLevel > 5) return -1;
    MemBitmap *bitmap = (MemBitmap *)VA_MEM_BMP;

    uint64_t i = curLevel != 5 ? idx[curLevel + 1] * 8 : 0;
    for (; i < BMP_SIZE_OF(curLevel) * BMP_JUMP; i++) {
        if (!(bitmap->whole[bmpGetOffset[(curLevel)] + i / BMP_JUMP].value & (1 << (i % BMP_JUMP)))) {
            idx[curLevel] = i;
            PhysAddr addr = i * BMP_MEM_SIZE_OF(curLevel);
            if (size == curLevel) {
                uint8_t valid = 0;
                for (uint8_t j = 0; j < count; j++)
                    if (curLevel == 0 || checkMem(curLevel, idx[curLevel] + j))
                        ++valid;
                if (valid == count) {
                    for (uint8_t j = 0; j < count; j++) {
                        bitmap->whole[bmpGetOffset[(curLevel)] + (i + j) / BMP_JUMP].value |= (1 << ((i + j) % BMP_JUMP));
                        rippleBitFlip(1, curLevel, idx);
                    }
                    return addr;
                }
            } else {
                PhysAddr addr = _resPhysMemory(size, count, curLevel - 1, idx);
                if (addr != -1UL) return addr;
            }
        }
    }
    return -1;
}

__attribute_no_vectorize__
PhysAddr resPhysMemory(uint8_t size, uint64_t count) {
    uint64_t idx[6];
    for (uint8_t i = 0; i < 6; i++) idx[i] = 0;
    return _resPhysMemory(size, count, 5, idx);
}

void freePhysMemory(PhysAddr ptr, uint8_t level) {
    uint64_t idx[6] = {0};
    idx[0] = ptr >> 12;
    for (uint8_t i = 1; i < 6; i++) {
        idx[i] = idx[i-1]/BMP_JUMP;
    }

    MemBitmap *bitmap = (MemBitmap *)VA_MEM_BMP;
    bitmap->whole[bmpGetOffset[(level)] + idx[level]/BMP_JUMP].value &= ~(1<<idx[level]%BMP_JUMP);
    rippleBitFlip(0, level, idx);
}

static void clearPT(PhysAddr phys) {
    PT(510, 509, 510)[0].whole = (uint64_t)(((uintptr_t)(phys & PTE_ADDR)) | PTE_P | PTE_RW | PTE_NX);
    invlpg((uint64_t)TEMP_PT(0));
    CLEAR_PT((PageEntry *)TEMP_PT(0));
    PT(510, 509, 510)[0].present = 0;
    invlpg((uint64_t)TEMP_PT(0));
}

int _mapPage(VirtAddr *out, PhysAddr phys, PageType target, uint64_t flags, uint16_t idx[4], uint8_t curDepth) {
    kprintf("target: %u, curDepth: %u, idx: %u %u %u %u\n", target, curDepth, idx[0], idx[1], idx[2], idx[3]);
    if ((uint8_t)curDepth == (uint8_t)target) {
        switch (target) {
            case PTE_PML4:
                kputs("[ERROR][_mapPage] Invalid level for page mapping\n");
                return 1;
            case PTE_PDP:
                ((PageEntry *)PDPT(idx[0]))[idx[1]].whole = (uint64_t)((phys & PTE_ADDR) | PTE_P | PTE_PS | flags);
                break;
            case PTE_PD:
                ((PageEntry *)PD(idx[0], idx[1]))[idx[2]].whole = (uint64_t)((phys & PTE_ADDR) | PTE_P | PTE_PS | flags);
                break;
            case PTE_PT:
                ((PageEntry *)PT(idx[0], idx[1], idx[2]))[idx[3]].whole = (uint64_t)((phys & PTE_ADDR) | PTE_P | flags);
                break;
        }
        return 0; // mapped page successfully
    }

    PageEntry *table;
    switch (curDepth) {
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
            kprintf("[ERROR][_mapPage] Invalid level to map page\n");
            CRIT_HLT();
    }
    for (uint16_t i = 0; i < 500; i++) {
        if (!table[i].present) {
            PhysAddr newPagePhys = resPhysMemory(BMP_MEM_4K, 1);
            clearPT(newPagePhys);
            table[i].whole = ((uint64_t)newPagePhys & PTE_ADDR) | PTE_P | PTE_RW;
        } else if (table[i].pageSize) continue;
        idx[curDepth] = i;
        if (!_mapPage(out, phys, target, flags, idx, curDepth + 1)) {
            *out = ((uint64_t)idx[0] << 39) | ((uint64_t)idx[1] << 30) | ((uint64_t)idx[2] << 21) | ((uint64_t)idx[3] << 12);
            return 0; // found an empty slot
        }
    }
    idx[curDepth] = 0;
    return 1; // failed to map
}

int mapPage(VirtAddr *out, PhysAddr addr, PageType type, uint64_t flags) {
    // PML4 idx, PDPT idx, PD idx, PT idx
    uint16_t idx[4] = {0};
    return _mapPage(out, addr, type, flags | PTE_US | PTE_RW, idx, 0);
}

int kmapPage(VirtAddr *out, PhysAddr addr, PageType type, uint64_t flags) {
    uint16_t idx[4] = {510};
    int result = _mapPage(out, addr, type, flags | PTE_RW, idx, 1);
    *out |= KERNEL_CANONICAL;
    return result;
}

void *kallocPage(uint8_t size) {
    register uint8_t physLevel = (size == MEM_2M ? BMP_MEM_2M : BMP_MEM_4K);
    PhysAddr phys = resPhysMemory(physLevel, 1);
    if (phys == -1UL) return NULL;

    VirtAddr virt;
    if (kmapPage(&virt, phys, size, 0UL)) {
        freePhysMemory(phys, physLevel);
        return NULL;
    }
    return (void *)virt;
}

void kfreePage(void *ptr) {
    uint8_t ptLevel;
    PhysAddr phys = getMapping((VirtAddr)ptr, &ptLevel);
    if (phys == -1UL) {
        kputs("[ERROR][kfreePage] Failed to free page: invalid mapping\n");
        return;
    }
    freePhysMemory(phys, ptLevel == MEM_2M ? BMP_MEM_2M : BMP_MEM_4K);
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
