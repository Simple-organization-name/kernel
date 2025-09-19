#ifndef __BMFT_H__
#define __BMFT_H__

#include <stdint.h>

#define BMFT_VERSION    (uint32_t)0x0100
#define BMFT_MAGIC      (uint32_t)0x74626D66  // 'bmft' as hex (little endian)

#define PRINTABLE_ASCII_FIRST   33 // Space doesn't count

typedef struct _Glyph {
    uint8_t     ascii;          // ascii code
    uint8_t     px12[12];       // 8x12 font
    uint16_t    px18[18];       // 12x18 font (16x18)
} Glyph;

typedef struct _Bmft {
    uint32_t    magicNumber;
    uint16_t    version;
    Glyph       glpyhs[UINT8_MAX - PRINTABLE_ASCII_FIRST + 1]; // Printable + Extended ASCII
} Bmft;

void loadBMFT();

#endif //__BMFT_H__