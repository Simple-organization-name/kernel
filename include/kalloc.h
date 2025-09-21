#ifndef __KALLOC_H__
#define __KALLOC_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "boot.h"
#include "memTables.h"

#define RECURSIVE_BASE 0xFFFFFF8000000000ULL
#define RECURSIVE_SLOT 511ULL

// Standard recursive page table mapping formulas
#define PML4() ((pte_t*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (RECURSIVE_SLOT << 21) | (RECURSIVE_SLOT << 12)))
#define PDPT(i) ((pte_t*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | (RECURSIVE_SLOT << 21) | ((i) << 12)))
#define PD(i, j) ((pte_t*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30) | ((i) << 21) | ((j) << 12)))
#define PT(i, j, k) ((pte_t*)(RECURSIVE_BASE | (RECURSIVE_SLOT << 39) | ((i) << 30) | ((j) << 21) | ((k) << 12)))

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
    size_t  size;
    void    *ptr;
    struct _MemBlock *next;
} MemBlock;

extern volatile MemMap  *physMemoryMap;
extern volatile MemoryRange validMemory[512];
extern volatile uint64_t validMemoryCount;

void initPhysMem();

bool mapPage(physAddr physical, virtAddr virtual);
bool unmapPage(virtAddr virtual);
physAddr getMapping(virtAddr virtual);

#endif
