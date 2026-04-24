#ifndef __MEMTABLES_H__
#define __MEMTABLES_H__

#include <stdint.h>

#define CLEAR_PT(PT) for (uint32_t _ = 0; _ < 512; _++) (PT)[_].whole = 0

// to manipulate one field by one
typedef union _PageEntry {
    uint64_t whole;
    struct {
    uint64_t present    : 1,    // is mapped
            rw          : 1,    // is writable
            us          : 1,    // can userspace code access it
            pwt         : 1,    
            pcd         : 1,    // if set, will not be cached
            accessed    : 1,    // has it been used in the mapping of a virtAddr
            dirty       : 1,    // has it been written to
            pageSize    : 1,    // is leaf / ps / &
            global      : 1,    // if set, not uncached when switching cr3 (for example kernel mapping that is the same in all processes)
        // Custom flags
            sreserved   : 1,    // reserved for special usage, page cannot be used
            reserved    : 1,    // is reserved not already mapped, see findEmpty...PageIdx functions
        // End of custom flags
            avl_1       : 1,
            dest        : 40,   // if leaf, where page starts. else, where next pageTable is
            avl_2       : 7,
            pk          : 4,    // protection key, see https://wiki.osdev.org/Paging#Page_Map_Table_Entries
            xd          : 1;    // if set, cannot execute page
    };
} PageEntry;

// to OR things together

#define PTE_P       (1ULL<<0)   // mapped
#define PTE_RW      (1ULL<<1)   // read/write
#define PTE_US      (1ULL<<2)   // user mode
#define PTE_PWT     (1ULL<<3)   // cache write through or whatever that means
#define PTE_PCD     (1ULL<<4)   // cache disable
#define PTE_A       (1ULL<<5)   // accessed
#define PTE_D       (1ULL<<6)   // dirty, automatically set by cpu if page had been written
#define PTE_PS      (1ULL<<7)   // leaf node
#define PTE_G       (1ULL<<8)   // global

// custom bits
#   define PTE_SR   (1ULL<<9) | (1ULL<<10)  // special/system reserved
#   define PTE_R    (1ULL<<10)              // reserved

#define PTE_NX      (1ULL<<63)  // not exec
#define PTE_ADDR    (0x000FFFFFFFFFF000)

#define MAKE_PAGE_ENTRY(addr, flags) ((uint64_t)(((uintptr_t)(addr) & PTE_ADDR) | PTE_P | ((uint64_t)(flags))))
// Check if the slot in the table is available, i.e. not `PTE_P`, `PTE_R`, `PTE_SR` flags not toggled
#define PAGE_TABLE_SLOT_AVAILABLE(entry) (!(entry->present || entry->sreserved || entry->reserved))

#endif
