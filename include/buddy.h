#ifndef __BUDDY_H__
#define __BUDDY_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "boot/bootInfo.h"
#include "memTables.h"
#include "kmemory.h"

#define BUDDY_MAX_ORDER 10

#define BUDDY_SIZE(level)                       ((1 << (level)) * (1 << 12))
#define BUDDY_PAIR_ID(level, addr)              (addr >> (level + 12 + 1))
#define BUDDY_STATE(table, level, addr)         (table->levels[level].map[BUDDY_PAIR_ID(level, addr) / 64] &  (1 << (BUDDY_PAIR_ID(level, addr) % 64)))
#define BUDDY_TOGGLE_BIT(table, level, addr)    (table->levels[level].map[BUDDY_PAIR_ID(level, addr) / 64] ^= (1 << (BUDDY_PAIR_ID(level, addr) % 64)))

typedef struct _Buddy {
    PhysAddr        start;
    struct _Buddy   *next;  
} Buddy;

typedef struct _BuddyArray {
    Buddy           *list;
    uint64_t        *map;
} BuddyLevel;

typedef struct _BuddyTable {
    BuddyLevel      levels[BUDDY_MAX_ORDER];
    Buddy           *usable;
    uint64_t        totalRAM;
} BuddyTable;

void initBuddy(EfiMemMap *physMemMap);

#endif
