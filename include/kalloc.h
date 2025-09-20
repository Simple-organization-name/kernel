#ifndef __KALLOC_H__
#define __KALLOC_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "boot.h"
#include "memTables.h"

typedef uint64_t physAddr;
typedef uint64_t virtAddr;

extern volatile pte_t   *pml4;
extern volatile MemMap  *physMemoryMap;

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

void initPhysMem();

bool mapPage(physAddr physical, virtAddr virtual);
bool unmapPage(virtAddr virtual);
physAddr getMapping(virtAddr virtual);

#endif
