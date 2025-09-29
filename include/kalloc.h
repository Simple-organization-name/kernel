#ifndef __KALLOC_H__
#define __KALLOC_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "boot.h"
#include "memTables.h"

#define RECURSIVE_BASE 0xFFFFFF8000000000UL
#define RECURSIVE_SLOT 511UL

// Standard recursive page table mapping formulas
#define PML4()      ((pte_t*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (RECURSIVE_SLOT << 21) | (RECURSIVE_SLOT << 12)))
#define PDPT(i)     ((pte_t*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (RECURSIVE_SLOT << 21) | ((i) << 12)))
#define PD(i, j)    ((pte_t*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | ((i) << 21) | ((j) << 12)))
#define PT(i, j, k) ((pte_t*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | ((i) << 30) | ((j) << 21) | ((k) << 12)))

// Memory bitmap
#define memoryBitmap_va 0xFFFFFF7FBFE00000

#define BMP_JUMP_POW2       3
#define BMP_JUMP            (1<<BMP_JUMP_POW2)
#define BMP_SIZE_OF(N)      (uint64_t)(((uint8_t)(N) < 6) ? 2<<(19-(BMP_JUMP_POW2*(uint8_t)(N))) : 0)
#define BMP_SIZE            (uint64_t)(BMP_SIZE_OF(0) + BMP_SIZE_OF(1) + BMP_SIZE_OF(2) + BMP_SIZE_OF(3) + BMP_SIZE_OF(4) + BMP_SIZE_OF(5))
#define BMP_MEM_SIZE_OF(N)  (uint64_t)(((uint8_t)(N) < 6) ? 2<<(11 + 3*(uint8_t)(N)) : 0)

#define MEM_BMP_PAGE_TABLE_START(BMP_VA)    (((uint64_t)(BMP_VA) + BMP_SIZE + 0xFFF) & ~0xFFF)
#define MEM_BMP_PAGE_TABLE_END(BMP_VA)      (((uint64_t)(BMP_VA) + (2<<20) - 1) & ~0xFFF)
#define MEM_BMP_PAGE_TABLE_SIZE(BMP_VA)     ((MEM_BMP_PAGE_TABLE_END((uint64_t)(BMP_VA)) - MEM_BMP_PAGE_TABLE_START((uint64_t)(BMP_VA)))/4096)

// Address types
#define PHYSICAL
#define VIRTUAL
typedef uint64_t PhysAddr;
typedef uint64_t VirtAddr;

typedef enum _PhysMemSize {
    MEM_4K      = 0,
    MEM_32K     = 1U,
    MEM_256K,
    MEM_2M,
    MEM_16M,
    MEM_128M,
} PhysMemSize;

typedef enum _PageType {
    PTE_PT,
    PTE_PD,
    PTE_PDP,
} PageType;

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiMaxMemoryType
} PhysicalMemoryType;

typedef struct _MemoryRange {
    PhysAddr    start;
    size_t      size;
} MemoryRange;

typedef struct _MemBlock {
    size_t              size;
    VirtAddr            ptr;
    struct _MemBlock    *next;
} MemBlock;

typedef union _BitmapValue {
    uint8_t value;
    struct {
    uint8_t bit1: 1,
            bit2: 1,
            bit3: 1,
            bit4: 1,
            bit5: 1,
            bit6: 1,
            bit7: 1,
            bit8: 1;
    };
} BitmapValue;

typedef union _MemBitmap {
    BitmapValue whole[BMP_SIZE];
    struct {
        BitmapValue level0[BMP_SIZE_OF(0)];
        BitmapValue level1[BMP_SIZE_OF(1)];
        BitmapValue level2[BMP_SIZE_OF(2)];
        BitmapValue level3[BMP_SIZE_OF(3)];
        BitmapValue level4[BMP_SIZE_OF(4)];
        BitmapValue level5[BMP_SIZE_OF(5)];
    };
} MemBitmap;

extern volatile MemMap *physMemoryMap;

void initPhysMem();
void printMemBitmapLevel(uint8_t n);
void printMemBitmap();
PhysAddr resPhysMemory(uint8_t size);

VirtAddr allocVirtMemory(uint8_t size, uint64_t count);

bool mapPage(PhysAddr physical, VirtAddr virtual, PageType page);
bool unmapPage(VirtAddr virtual);
PhysAddr getMapping(VirtAddr virtual, uint8_t *pageLevel);

#endif
