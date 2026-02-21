#include "kvmalloc.h"
#include "memmap.h"
#include "buddy.h"
#include "asm.h"

inline void *__kvmalloc(uint16_t *idx, size_t nbPages, uint64_t flags) {
    if (findEmptyRangePageIdx(PTE_PT, idx, nbPages) != nbPages) {
        return NULL;
    }
    void *ptr = VA_ARRAY(idx);
    for (size_t i = 0; i < nbPages; i++) {
        PhysAddr phys = buddyAlloc(BUDDY_4K);
        if (!mapPage(idx, PTE_PT, phys, PTE_RW | flags)) {
            // TODO: Free physical pages and reserved virtual pages
            return NULL;
        }
        idx[3]++;
        for (uint8_t j = 3; j > 0; j--) {
            if (idx[j] != 512) break;
            idx[j] = 0;
            idx[j-1]++;
        }
        if (idx[0] >= 512) {
            PRINT_ERR("Found range exceeds PML4[512] ???\n");    
            CRIT_HLT();
        }
    }

    return ptr;
}

/**
 * Allocate memory
 * Maps physical pages to virtual memory
 */
inline void *_kvmalloc(size_t nbPages, uint64_t flags) {
    uint16_t idx[4] = {1, 0, 0, 0};
    return __kvmalloc(idx, nbPages, flags);
}
