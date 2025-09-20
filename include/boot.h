#ifndef __BOOT_H__
#define __BOOT_H__

#include <stdint.h>

#define HLT __asm__("cli\nhlt")

typedef struct _Framebuffer {
    uint64_t    addr,
                size;
    uint32_t    width,
                height,
                pitch;

} Framebuffer;

typedef struct _MemoryDescriptor {
    uint32_t    Type;
    uint32_t    Pad;
    uint64_t    PhysicalStart;
    uint64_t    VirtualStart;
    uint64_t    NumberOfPages;
    uint64_t    Attribute;
} MemoryDescriptor;

typedef struct _MemMap {
    uint64_t            count;
    uint64_t            mapSize;
    uint64_t            descSize;
    MemoryDescriptor    *map;
    uint64_t            key;
} MemMap;

typedef struct _Pixel {
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Pixel;

typedef struct _fileData {
    void* data;
    uint64_t size;
} FileData;

typedef struct _fileArray {
    FileData config;
    FileData* files;
    uint64_t count;
} FileArray;

typedef struct _BootInfo {
    Framebuffer* frameBuffer;
    MemMap* memMap;
    FileArray* files;
} BootInfo;


#endif //__BOOT_H__
