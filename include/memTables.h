#ifndef __MEMTABLES_H__
#define __MEMTABLES_H__

#include <stdint.h>

#define CLEAR_PT(PT) for (uint32_t _ = 0; _ < 512; _++) (PT)[_].whole = 0

// to manipulate one field by one
typedef union _PageEntry {
    uint64_t whole;
    struct {
    uint64_t present : 1,   // is used
            rw : 1,         // is writable
            us : 1,         // can userspace code access it
            pwt : 1,
            pcd : 1,        // if set, will not be cached
            accessed : 1,   // has it been used in the mapping of a virtAddr
            dirty : 1,      // has it been written to
            pageSize : 1,   // is leaf / ps / whatever
            global : 1,     // if set, not uncached when switching cr3 (for example kernel mapping that is the same in all processes)
            avl_1 : 3,
            dest : 40,      // if leaf, where page starts. else, where next pageTable is
            avl_2 : 11,
            xd : 1;         // if set, cannot execute page
    };
} PageEntry;

// to OR things together

#define PTE_P   (1UL<<0)   // used
#define PTE_RW  (1UL<<1)   // read/write
#define PTE_US  (1UL<<2)   // user mode
#define PTE_PWT (1UL<<3)   // cache write through or whatever that means
#define PTE_PCD (1UL<<4)   // cache disable
#define PTE_A   (1UL<<5)   // accessed
#define PTE_D   (1UL<<6)   // dirty
#define PTE_PS  (1UL<<7)   // leaf node
#define PTE_G   (1UL<<8)   // global
#define PTE_NX  (1UL<<63)  // not exec
#define PTE_ADDR (0x000FFFFFFFFFF000)

#endif
