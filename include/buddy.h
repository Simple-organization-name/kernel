#ifndef __BUDDY_H__
#define __BUDDY_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "boot/bootInfo.h"
#include "memTables.h"
#include "kmemory.h"

#define BUDDY_MAX_ORDER 10

#define BUDDY_SIZE(level) ((1 << (level)) * (1 << 12))

typedef struct _Buddy {
    PhysAddr        start;
    struct _Buddy   *next;
} Buddy;

typedef struct _BuddyArray {
    Buddy           *mem;
    uint64_t        *map; // 
} BuddyLevel;

typedef struct {
    BuddyLevel      levels[BUDDY_MAX_ORDER];
    Buddy           *usable;
} BuddyTable;

void initBuddy(EfiMemMap *physMemMap);

#endif
