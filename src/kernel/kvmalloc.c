#include "kvmalloc.h"
#include "memmap.h"

inline void *kvmalloc(size_t nbPages) {
    size_t ret = 0;
    uint16_t idx[4] = {0, 0, 0, 0};
    if (findEmptyRangePageIdx(PTE_PT, idx, nbPages) != nbPages) {
        return 0;
    }
}
