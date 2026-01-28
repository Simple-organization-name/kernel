#include <kmap.h>
#include <kmemory.h>
#include <buddy.h>
#include <kerror.h>
#include <asm.h>

static int _mapPage(VirtAddr *out, PhysAddr phys, PageType target, uint64_t flags, uint16_t idx[4], uint8_t curDepth) {
    kprintf("target: %u, curDepth: %u, idx: %u %u %u %u\n", target, curDepth, idx[0], idx[1], idx[2], idx[3]);
    if ((uint8_t)curDepth == (uint8_t)target) {
        switch (target) {
            case PTE_PML4:
                PRINT_ERR("Invalid level for page mapping\n");
                return 0;
            case PTE_PDP:
                ((PageEntry *)PDPT(idx[0]))[idx[1]].whole = (uint64_t)((phys & PTE_ADDR) | PTE_P | PTE_PS | flags);
                break;
            case PTE_PD:
                ((PageEntry *)PD(idx[0], idx[1]))[idx[2]].whole = (uint64_t)((phys & PTE_ADDR) | PTE_P | PTE_PS | flags);
                break;
            case PTE_PT:
                ((PageEntry *)PT(idx[0], idx[1], idx[2]))[idx[3]].whole = (uint64_t)((phys & PTE_ADDR) | PTE_P | flags);
                break;
        }
        kprintf("Mapped page at %u %u %u %u\n", idx[0], idx[1], idx[2], idx[3]);
        return 1; // mapped page successfully
    }

    PageEntry *table;
    switch (curDepth) {
        case PTE_PML4: // Searching for entry in pml4
            table = PML4();
            break;
        case PTE_PDP: // ... for entry in pdp
            table = PDPT(idx[0]);
            break;
        case PTE_PD: // ... for entry in pd
            table = PD(idx[0], idx[1]);
            break;
        case PTE_PT: // ... for entry in pt
            table = PT(idx[0], idx[1], idx[2]);
            break;
        default:
            PRINT_ERR("Invalid level to map page\n");
            CRIT_HLT();
    }
    for (uint16_t i = 0; i < (curDepth == PTE_PML4 ? 510 : 511); i++) {
        if (!table[i].present) {
            PhysAddr newPagePhys = buddyAlloc(0);
            clearPageTable(newPagePhys);
            table[i].whole = ((uint64_t)newPagePhys & PTE_ADDR) | PTE_P | PTE_RW;
        } else if (table[i].pageSize) continue;
        idx[curDepth] = i;
        if (_mapPage(out, phys, target, flags, idx, curDepth + 1)) {
            *out = ((uint64_t)idx[0] << 39) | ((uint64_t)idx[1] << 30) | ((uint64_t)idx[2] << 21) | ((uint64_t)idx[3] << 12);
            return 1; // found an empty slot
        }
    }
    idx[curDepth] = 0;
    return 0; // failed to map
}

int mapPage(VirtAddr *out, PhysAddr addr, PageType type, uint64_t flags) {
    // PML4 idx, PDPT idx, PD idx, PT idx
    uint16_t idx[4] = {0};
    return _mapPage(out, addr, type, flags | PTE_US | PTE_RW, idx, 0);
}

int kmapPage(VirtAddr *out, PhysAddr addr, PageType type, uint64_t flags) {
    uint16_t idx[4] = {510};
    int result = _mapPage(out, addr, type, flags | PTE_RW, idx, 1);
    *out |= KERNEL_CANONICAL;
    return result;
}
