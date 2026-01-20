#include "PCI.h"
#include "kterm.h"
#include "asm.h"

uint32_t PCI_readConfigRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t reg)
{
    // select the register according to arguments
    outl(
        PCI_CONFIG_ADDRESS,
        PCI_CONSTRUCT_CONFIG_ADDRESS(bus, slot, func, reg * 4)
    );
    return inl(PCI_CONFIG_DATA);
}

void PCI_printAll()
{
    for (int bus = 0; bus < 256; bus++)
    for (int device_slot = 0; device_slot < 32; device_slot++)
    for (int func = 0; func < 8; func++)
    {
        uint32_t reg0 = PCI_readConfigRegister(bus, device_slot, func, 0);
        if ((reg0 & 0xFFFF) != 0xFFFF) // if vendor_id != invalid
        {
            uint32_t reg2 = PCI_readConfigRegister(bus, device_slot, func, 8);
            kprintf(
                "[PCI::0x%x::0x%x::0x%x] VendorID=0x%x; DeviceID=0x%x; Class=0x%x; Subclass=0x%x; ProgIF=0x%x\n",
                bus, device_slot, func, reg0 & 0xFFFF, reg0 >> 16, reg2 >> 24, (reg2 >> 16) & 0xFF, (reg2 >> 8) & 0xFF
            );
        }
    }
}

int PCI_findOfType(int class, int subclass, int progif, int max, PciDevice *buf)
{
    if (!buf) return -1;
    int matched = 0;
    PciDevice tmp;
    for (int bus = 0; bus < 256; bus++)
    for (int device_slot = 0; device_slot < 32; device_slot++)
    for (int func = 0; func < 8; func++)
    {   
        uint32_t reg2 = PCI_readConfigRegister(bus, device_slot, func, 8);
        tmp.class = reg2 >> 24;
        tmp.subclass = (reg2 >> 16) & 0xFF;
        tmp.prog_if = (reg2 >> 8) & 0xFF;
        if (
            (tmp.class == class || class < 0) &&
            (tmp.class == class || subclass < 0) &&
            (tmp.class == class || progif < 0)
        ) {
            matched++;
            if (matched <= max) {
                tmp.bus = bus; tmp.device = device_slot; tmp.function = func;
                
                tmp.revisionID = reg2 & 0xFF;
                uint32_t bar0 = PCI_readConfigRegister(bus, device_slot, func, 0);
                tmp.deviceID = bar0 >> 16;
                tmp.vendorID = bar0 & 0xFFFF;
                buf[matched - 1] = tmp;
            }
        }
    }
    return matched;
}
