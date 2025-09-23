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
#define PML4() ((pte_t*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (RECURSIVE_SLOT << 21) | (RECURSIVE_SLOT << 12)))
#define PDPT(i) ((pte_t*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (RECURSIVE_SLOT << 21) | ((i) << 12)))
#define PD(i, j) ((pte_t*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | ((i) << 21) | ((j) << 12)))
#define PT(i, j, k) ((pte_t*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | ((i) << 30) | ((j) << 21) | ((k) << 12)))

#define memoryBitmap_va 0xFFFFFF7FBFE00000

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

#define BITMAP_LEVEL_JUMP   8
#define BITMAP_LEVEL1_SIZE  (2<<19)
#define BITMAP_LEVEL2_SIZE  (2<<16)
#define BITMAP_LEVEL3_SIZE  (2<<13)
#define BITMAP_LEVEL4_SIZE  (2<<10)
#define BITMAP_LEVEL5_SIZE  (2<<8)
#define BITMAP_LEVEL6_SIZE  (2<<6)
#define MEM_BITMAP_SIZE     (BITMAP_LEVEL1_SIZE + BITMAP_LEVEL2_SIZE + BITMAP_LEVEL3_SIZE + BITMAP_LEVEL4_SIZE + BITMAP_LEVEL5_SIZE + BITMAP_LEVEL6_SIZE)
typedef union _MemBitmap {
    BitmapValue whole[MEM_BITMAP_SIZE];
    struct {
        BitmapValue level1[BITMAP_LEVEL1_SIZE];
        BitmapValue level2[BITMAP_LEVEL2_SIZE];
        BitmapValue level3[BITMAP_LEVEL3_SIZE];
        BitmapValue level4[BITMAP_LEVEL4_SIZE];
        BitmapValue level5[BITMAP_LEVEL5_SIZE];
        BitmapValue level6[BITMAP_LEVEL6_SIZE];
    };
} MemBitmap;

extern volatile MemMap  *physMemoryMap;

void initPhysMem();

bool mapPage(physAddr physical, virtAddr virtual);
bool unmapPage(virtAddr virtual);
physAddr getMapping(virtAddr virtual, uint8_t *pageLevel);

#endif 
