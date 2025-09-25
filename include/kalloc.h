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
#define BMP_LEVEL_JUMP  8
#define BMP_L1_SIZE     (2<<19)
#define BMP_L2_SIZE     (BMP_L1_SIZE/BMP_LEVEL_JUMP)
#define BMP_L3_SIZE     (BMP_L2_SIZE/BMP_LEVEL_JUMP)
#define BMP_L4_SIZE     (BMP_L3_SIZE/BMP_LEVEL_JUMP)
#define BMP_L5_SIZE     (BMP_L4_SIZE/BMP_LEVEL_JUMP)
#define BMP_L6_SIZE     (BMP_L5_SIZE/BMP_LEVEL_JUMP)
#define BMP_SIZE        (BMP_L1_SIZE + BMP_L2_SIZE + BMP_L3_SIZE + BMP_L4_SIZE + BMP_L5_SIZE + BMP_L6_SIZE)

#define BMP_L1_MEM_SIZE 4096
#define BMP_L2_MEM_SIZE (BMP_L1_MEM_SIZE * BMP_LEVEL_JUMP)
#define BMP_L3_MEM_SIZE (BMP_L2_MEM_SIZE * BMP_LEVEL_JUMP)
#define BMP_L4_MEM_SIZE (BMP_L3_MEM_SIZE * BMP_LEVEL_JUMP)
#define BMP_L5_MEM_SIZE (BMP_L4_MEM_SIZE * BMP_LEVEL_JUMP)
#define BMP_L6_MEM_SIZE (BMP_L5_MEM_SIZE * BMP_LEVEL_JUMP)

#define memoryBitmap_va                     0xFFFFFF7FBFE00000
#define MEM_BMP_PAGE_TABLE_START(BMP_VA)    (((uint64_t)(BMP_VA) + BMP_SIZE + 0xFFF) & ~0xFFF)
#define MEM_BMP_PAGE_TABLE_END(BMP_VA)      (((uint64_t)(BMP_VA) + (2<<20) - 1) & ~0xFFF)
#define MEM_BMP_PAGE_TABLE_SIZE(BMP_VA)     ((MEM_BMP_PAGE_TABLE_END((uint64_t)(BMP_VA)) - MEM_BMP_PAGE_TABLE_START((uint64_t)(BMP_VA)))/4096)

// Address types
#define PHYSICAL
#define VIRTUAL
typedef uint64_t physAddr;
typedef uint64_t virtAddr;

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
    physAddr    start;
    size_t      size;
} MemoryRange;

typedef struct _MemBlock {
    size_t              size;
    virtAddr            ptr;
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
        BitmapValue level1[BMP_L1_SIZE];
        BitmapValue level2[BMP_L2_SIZE];
        BitmapValue level3[BMP_L3_SIZE];
        BitmapValue level4[BMP_L4_SIZE];
        BitmapValue level5[BMP_L5_SIZE];
        BitmapValue level6[BMP_L6_SIZE];
    };
} MemBitmap;

extern volatile MemMap *physMemoryMap;

void initPhysMem();
void printMemBitmap();
physAddr resPhysMemory(size_t size);

bool mapPage(physAddr physical, virtAddr virtual);
bool unmapPage(virtAddr virtual);
physAddr getMapping(virtAddr virtual, uint8_t *pageLevel);

#endif 
