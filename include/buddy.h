#ifndef __BUDDY_H__
#define __BUDDY_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "boot/bootInfo.h"
#include "memTables.h"
#include "kmemory.h"

#define BUDDY_MAX_ORDER 10

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
PhysAddr buddyAlloc(uint8_t level);
void buddyFree(uint8_t level, PhysAddr addr);

#endif
