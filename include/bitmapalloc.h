#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "boot/bootInfo.h"
#include "memTables.h"
#include "kmemory.h"

// Memory bitmap
#define VA_MEM_BMP  0xFFFFFF7F7FE00000UL
#define TEMP_PT(i)  ((void *)(KERNEL_CANONICAL | (510UL << 39) | (509UL << 30) | (510UL << 21) | ((uint64_t)(i) << 12)))

#define BMP_MEM_4K      0U
#define BMP_MEM_32K     1U
#define BMP_MEM_256K    2U
#define BMP_MEM_2M      3U
#define BMP_MEM_16M     4U
#define BMP_MEM_128M    5U

#define BMP_JUMP_POW2       3
#define BMP_JUMP            (1<<BMP_JUMP_POW2)
// Size of a bitmap level
#define BMP_SIZE_OF(N)      (uint64_t)(((uint8_t)(N) < 6) ? 2<<(19-(BMP_JUMP_POW2*(uint8_t)(N))) : 0)
// Size of the whole bitmap
#define BMP_SIZE            (uint64_t)(BMP_SIZE_OF(0) + BMP_SIZE_OF(1) + BMP_SIZE_OF(2) + BMP_SIZE_OF(3) + BMP_SIZE_OF(4) + BMP_SIZE_OF(5))
// Size of a page at level N
#define BMP_MEM_SIZE_OF(N)  (uint64_t)(((uint8_t)(N) < 6) ? 2<<(11 + 3*(uint8_t)(N)) : 0)

#define MEM_4K ((uint8_t)PTE_PT)
#define MEM_2M ((uint8_t)PTE_PD)

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

void initPhysMem(EfiMemMap *physMemMap);
PhysAddr resPhysMemory(uint8_t size, uint64_t count);
void freePhysMemory(PhysAddr ptr, uint8_t level);
void printMemBitmapLevel(uint8_t n);
void printMemBitmap();

// Kernel mapping
int kmapPage(VirtAddr *out, PhysAddr addr, PageType type, uint64_t flags);
void *kallocPage(uint8_t size);
void kfreePage(void *ptr);

#endif
