#include <stdint.h>
#include <stdio.h>
#include <string.h>

void print_va_breakdown(uint64_t va) {
    uint16_t pml4_idx = (va >> 39) & 0x1FF;
    uint16_t pdpt_idx = (va >> 30) & 0x1FF;
    uint16_t pd_idx = (va >> 21) & 0x1FF;
    uint16_t pt_idx = (va >> 12) & 0x1FF;
    uint16_t offset = va & 0xFFF;
    
    printf("VA: 0x%lX\n", va);
    printf("PML4[%d] PDPT[%d] PD[%d] PT[%d]\n", 
            pml4_idx, pdpt_idx, pd_idx, pt_idx);
}

void print_va(uint16_t pml4_idx, uint16_t pdpt_idx, uint16_t pd_idx, uint16_t pt_idx) {
    uint64_t va = ((uint64_t)pml4_idx << 39) | ((uint64_t)pdpt_idx << 30) | ((uint64_t)pd_idx << 21) | ((uint64_t)pt_idx << 12);
    if (va & (1ULL << 47)) {
        va |= 0xFFFF000000000000;
    }
    printf("VA: 0x%lX\n", va);
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++) {
        if (strcmp("-va", argv[i]) == 0) {
            uint64_t va;
            if (++i >= argc) return 1;
            sscanf(argv[i], "%lX", &va);
            print_va_breakdown(va);
        }
        if (strcmp("-idx", argv[i]) == 0) {
            uint16_t idx[4];
            for (int j = 0; j < 4; j++) {
                if (++i >= argc) return 1;
                sscanf(argv[i], "%hu", &idx[j]);
            }
            print_va(idx[0], idx[1], idx[2], idx[3]);
        }
    }
    return 0;
}