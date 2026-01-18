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
    size_t          size;
    struct _Buddy   *next;
} Buddy;

typedef union BuddyInfo {
    uint8_t     whole;
    struct {
    uint8_t     free: 1;
    };
} BuddyInfo;

typedef struct _BuddyInfoArray {
    BuddyInfo       *info[BUDDY_MAX_ORDER];
} BuddyInfoArray;

typedef struct _BuddyArray {
    Buddy           *array[BUDDY_MAX_ORDER];
} BuddyArray, FreeBuddyArray;

void initBuddy(EfiMemMap *physMemMap);

#endif

