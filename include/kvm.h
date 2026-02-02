#ifndef __KVM_H__
#define __KVM_H__

#include <stdint.h>
#include <stddef.h>

#define VM_START (1<<21)

typedef struct _VirtMemory {
    void                *addr;
    size_t              size;
    struct _VirtMemory  *next;
} VirtMemory;

#endif
