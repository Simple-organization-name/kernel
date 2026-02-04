#ifndef __KVM_H__
#define __KVM_H__

#include <stdint.h>
#include <stddef.h>

#define VM_START_ARR    ((uint16_t[4]){2, 0, 0, 0})
#define VM_START        (0x10000000000UL)

#define VM_START 

typedef struct _VirtMemory {
    void                *addr;
    size_t              size;
    struct _VirtMemory  *next;
} VirtMemory;

size_t kvmalloc(size_t size);

#endif
