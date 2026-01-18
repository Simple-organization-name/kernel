#ifndef __BUDDY_H__
#define __BUDDY_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "boot/bootInfo.h"
#include "memTables.h"

#define KERNEL_CANONICAL 0xFFFF000000000000UL

#define RECURSIVE_BASE 0xFFFFFF8000000000UL
#define RECURSIVE_SLOT 511UL

// Standard recursive page table mapping formulas
#define PML4()      ((PageEntry*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (RECURSIVE_SLOT << 21) | (RECURSIVE_SLOT << 12)))
#define PDPT(i)     ((PageEntry*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (RECURSIVE_SLOT << 21) | ((uint64_t)(i) << 12)))
#define PD(i, j)    ((PageEntry*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (((uint64_t)i) << 21) | ((uint64_t)(j) << 12)))
#define PT(i, j, k) ((PageEntry*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | ((uint64_t)(i) << 30) | ((uint64_t)(j) << 21) | ((uint64_t)(k) << 12)))

// Address types
#define PHYSICAL
#define VIRTUAL
typedef uint64_t PhysAddr;
typedef uint64_t VirtAddr;

typedef enum _PageType {
    PTE_PML4,
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

#define BUDDY_MAX_ORDER 10

typedef struct _PhysMemRange {
    PhysAddr    start;
    size_t      size;
} PhysMemRange;

typedef struct _Buddy {
    PhysAddr        start;
    size_t          size;
    struct _Buddy   *next;
} Buddy;

typedef union BuddyInfo {
    uint8_t     whole;
    struct {
    uint8_t     free: 1;
    };
} BuddyInfo;

typedef struct _BuddyInfo {
    BuddyInfo       *info[BUDDY_MAX_ORDER];
} BuddyInfo;

typedef struct _BuddyArray {
    Buddy           *array[BUDDY_MAX_ORDER];
    BuddyInfo;
} BuddyArray, FreeBuddyArray;

#endif
