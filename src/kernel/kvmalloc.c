#include <stdatomic.h>

#include <kvmalloc.h>
#include <memmap.h>
#include <buddy.h>
#include <asm.h>
#include <kerror.h>

void *__kvmalloc(uint16_t *idx, size_t nbPages, uint64_t flags) {
    if (memmap_findRange(PTE_PT, idx, nbPages) != nbPages) {
        return NULL;
    }

    void *ptr = VA_ARRAY(idx);
    PRINT_DEBUG("Address if successfully mapped: %p\n", ptr);
    for (size_t i = 0; i < nbPages; i++) {
        PhysAddr phys = buddy_alloc(BUDDY_4K);
        if (!memmap_map(idx, PTE_PT, phys, PTE_RW | flags)) {
            PRINT_ERR("Failed to map page idx: {%u, %u, %u, %u}\n", idx[0], idx[1], idx[2], idx[3]);
            CRIT_HLT();
        }
        idx[3]++;
        for (uint8_t j = 3; j > 0; j--) {
            if (idx[j] != 512) break;
            idx[j] = 0;
            idx[j-1]++;
        }
        // kprintf("next idx: %u %u %u %u\n", idx[0], idx[1], idx[2], idx[3]);
        if (idx[0] >= 511) {
            PRINT_ERR("Found range exceeds PML4[511] ???\n");    
            CRIT_HLT();
        }
    }

    return ptr;
}

/**
 * Allocate memory
 * Maps physical pages to virtual memory
 */
void *_kvmalloc(size_t nbPages, uint64_t flags) {
    uint16_t idx[4] = {1, 0, 0, 0};
    return __kvmalloc(idx, nbPages, flags);
}
