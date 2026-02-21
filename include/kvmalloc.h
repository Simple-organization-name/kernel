#ifndef __KVMALLOC_H__
#define __KVMALLOC_H__

#include <stddef.h>
#include <stdint.h>

void *__kvmalloc(uint16_t *idx, size_t nbPages, uint64_t flags);
void *_kvmalloc(size_t nbPages, uint64_t flags);

/**
 * Allocate virtually mapped memory for kernel usage
 * \param idx `optionnal` - The starting index for the memory in the virtual mapping
 * \param nbPages The number of 4KiB pages to allocate
 * \param flags The flags for the mapping. `PTE_RW | PTE_P` is set.
 * \return The pointer to the memory
 */
#define kvmalloc(X, ...) _Generic((X), \
    uint16_t *  : __kvmalloc, \
    default     : _kvmalloc  \
) (X, __VA_ARGS__)

#endif
