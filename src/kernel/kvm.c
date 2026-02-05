// #include "kvm.h"
// #include "memory.h"
// #include "buddy.h"
// #include "memmap.h"

// /**
//  * Allocate the smallest block fitting size
//  * And map it in an empty slot
//  * \return The size of the block allocated
//  */
// size_t kvmalloc(size_t size) {
//     size_t totalAllocated = 0;
//     for (int8_t i = BUDDY_MAX_ORDER; i >= 0; i--) {
//         while (size > BUDDY_IDX_BLOCK_SIZE(i)) {
//             uint16_t idx[4] = {0};
//             if (!findEmptySlotPageIdx(PTE_PT, idx))
//                 return 0;

//             PhysAddr addr = buddyAlloc(i);
            
//         }
//     }
// }
