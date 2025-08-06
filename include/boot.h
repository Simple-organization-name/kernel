#ifndef __BOOT_H__
#define __BOOT_H__

#include <stdint.h>

typedef struct _Framebuffer {
    uint64_t    addr,
                size;
    uint32_t    width,
                height;
    
} Framebuffer;

typedef struct _MemoryDescriptor {
    uint32_t                        Type;
    uint32_t                        Pad;
    uint64_t                        PhysicalStart;
    uint64_t                        VirtualStart;
    uint64_t                        NumberOfPages;
    uint64_t                        Attribute;
} MemoryDescriptor;

typedef struct _MemMap {
    uint64_t                        count;
    uint64_t                        mapSize;
    uint64_t                        descSize;
    MemoryDescriptor                *map;
    uint64_t                        key;
} MemMap;

typedef struct _Pixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t reserved;
} Pixel;


#endif //__BOOT_H__
