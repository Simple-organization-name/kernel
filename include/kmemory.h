#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>
#include <stddef.h>

#include "memTables.h"

#define VA_TEMP_PT  0xFFFFFF7F7FC00000UL
#define KERNEL_CANONICAL 0xFFFF000000000000UL

#define RECURSIVE_BASE 0xFFFFFF8000000000UL
#define RECURSIVE_SLOT 511UL

// Standard recursive page table mapping formulas
#define PML4()      ((PageEntry*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (RECURSIVE_SLOT << 21) | (RECURSIVE_SLOT << 12)))
#define PDPT(i)     ((PageEntry*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (RECURSIVE_SLOT << 21) | ((uint64_t)(i) << 12)))
#define PD(i, j)    ((PageEntry*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (((uint64_t)i) << 21) | ((uint64_t)(j) << 12)))
#define PT(i, j, k) ((PageEntry*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | ((uint64_t)(i) << 30) | ((uint64_t)(j) << 21) | ((uint64_t)(k) << 12)))

#define VIRT_ADDR(pml4, pdpt, pd, pt) (((uint64_t)pml4 << 39) | ((uint64_t)pdpt << 30) | ((uint64_t)pd << 21) | ((uint64_t)pt << 12))

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

typedef struct _MemoryRange {
    PhysAddr    start;
    size_t      size;
} MemoryRange;

void *memset(void *dest, int val, size_t count);

// Mapping
int unmapPage(VirtAddr virtual);
PhysAddr getMapping(VirtAddr virtual, uint8_t *pageLevel);


#endif