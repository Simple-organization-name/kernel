#ifndef __MEMTABLES_H__
#define __MEMTABLES_H__

#include <stdint.h>

// to manipulate one field by one

typedef union _pageTableEntry {
    uint64_t whole;
    struct {
    uint64_t present : 1,   // is used
            rw : 1,         // is writable
            us : 1,         // can userspace code access it
            pwt : 1,
            pcd : 1,        // if set, will not be cached
            accessed : 1,   // has it been used in the mapping of a va
            dirty : 1,      // has it been written to
            pageSize : 1,   // is leaf / ps / whatever
            global : 1,     // if set, not uncached when switching cr3 (for example kernel mapping that is the same in all processes)
            avl_1 : 3,
            dest : 40,      // if leaf, where page starts. else, where next pageTable is
            avl_2 : 11,
            xd : 1;         // if set, cannot execute page
    };
} pte_t;

// to OR things together

#define PTE_P   (1ULL<<0)   // used
#define PTE_RW  (1ULL<<1)   // read/write
#define PTE_US  (1ULL<<2)   // user mode
#define PTE_PWT (1ULL<<3)   // cache write through or whatever that means
#define PTE_PCD (1ULL<<4)   // cache disable
#define PTE_A   (1ULL<<5)   // accessed
#define PTE_D   (1ULL<<6)   // dirty
#define PTE_PS  (1ULL<<7)   // leaf node
#define PTE_G   (1ULL<<8)   // global
#define PTE_NX  (1ULL<<63)  // not exec

#endif
