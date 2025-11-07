#ifndef __KALLOC_H__
#define __KALLOC_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "boot.h"
#include "memTables.h"

#define KERNEL_CANONICAL 0xFFFF000000000000UL

#define RECURSIVE_BASE 0xFFFFFF8000000000UL
#define RECURSIVE_SLOT 511UL

// Standard recursive page table mapping formulas
#define PML4()      ((PageEntry*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (RECURSIVE_SLOT << 21) | (RECURSIVE_SLOT << 12)))
#define PDPT(i)     ((PageEntry*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (RECURSIVE_SLOT << 21) | ((uint64_t)(i) << 12)))
#define PD(i, j)    ((PageEntry*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (((uint64_t)i) << 21) | ((uint64_t)(j) << 12)))
#define PT(i, j, k) ((PageEntry*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | ((uint64_t)(i) << 30) | ((uint64_t)(j) << 21) | ((uint64_t)(k) << 12)))

// Memory bitmap
#define VA_MEM_BMP  0xFFFFFF7F7FE00000
#define VA_TEMP_PT  0xFFFFFF7F7FC00000
#define TEMP_PT(i)  ((void *)(KERNEL_CANONICAL | (510UL << 39) | (509UL << 30) | (510UL << 21) | ((uint64_t)(i) << 12)))

#define MEM_4K      0
#define MEM_32K     1
#define MEM_256K    2
#define MEM_2M      3
#define MEM_16M     4
#define MEM_128M    5

#define BMP_JUMP_POW2       3
#define BMP_JUMP            (1<<BMP_JUMP_POW2)
// Size of a bitmap level
#define BMP_SIZE_OF(N)      (uint64_t)(2<<(19-(BMP_JUMP_POW2*(uint8_t)(N))))
// Size of the whole bitmap
#define BMP_SIZE            (uint64_t)(BMP_SIZE_OF(0) + BMP_SIZE_OF(1) + BMP_SIZE_OF(2) + BMP_SIZE_OF(3) + BMP_SIZE_OF(4) + BMP_SIZE_OF(5))
// Size of a page at level N
#define BMP_MEM_SIZE_OF(N)  (uint64_t)(((uint8_t)(N) < 6) ? 2<<(11 + 3*(uint8_t)(N)) : 0)

// Address types
#define PHYSICAL
#define VIRTUAL
typedef uint64_t PhysAddr;
typedef uint64_t VirtAddr;

typedef enum _PageType {
    PTE_PDP,    // PageDirectoryPointer:    1GiB
    PTE_PD,     // PageDirectory:           2MiB
    PTE_PT,     // PageTable:               4kiB
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

typedef int (*MapFunc)(VirtAddr *, PhysAddr, PageType, uint64_t);

void *memset(void *dest, int val, size_t count);

PhysAddr vaToPa(VirtAddr va, PageType type);

void initPhysMem(EfiMemMap *physMemMap);
PhysAddr resPhysMemory(uint8_t size, uint64_t count);
void freePhysMemory(PhysAddr ptr, uint8_t level);
void printMemBitmapLevel(uint8_t n);
void printMemBitmap();

// Kernel mapping
int kmapPage(VirtAddr *out, PhysAddr addr, PageType type, uint64_t flags);

// General mapping
int mapPage(VirtAddr *out, PhysAddr addr, PageType type, uint64_t flags);

int unmapPage(VirtAddr virtual);
PhysAddr getMapping(VirtAddr virtual, uint8_t *pageLevel);

// Allocation
//  yes

#endif
