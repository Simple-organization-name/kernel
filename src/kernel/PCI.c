#include "PCI.h"
#include "kterm.h"
#include "asm.h"

uint32_t pci_readConfigRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    // select the register according to arguments
    outl(
        PCI_CONFIG_ADDRESS,
        PCI_CONSTRUCT_CONFIG_ADDRESS(bus, slot, func, offset)
    );
    return inl(PCI_CONFIG_DATA);
}

void printAllPCI()
{
    for (int bus = 0; bus < 256; bus++)
    for (int device_slot = 0; device_slot < 32; device_slot++)
    for (int func = 0; func < 8; func++)
    {
        uint32_t reg0 = pci_readConfigRegister(bus, device_slot, func, 0);
        if ((reg0 & 0xFFFF) != 0xFFFF) // if vendor_id != invalid
        {
            uint32_t reg2 = pci_readConfigRegister(bus, device_slot, func, 8);
            kprintf(
                "[PCI::0x%x::0x%x::0x%x] VendorID=0x%x; DeviceID=0x%x; Class=0x%x; Subclass=0x%x; ProgIF=0x%x\n",
                bus, device_slot, func, reg0 & 0xFFFF, reg0 >> 16, reg2 >> 24, (reg2 >> 16) & 0xFF, (reg2 >> 8) & 0xFF
            );
        }
    }
}

int getDeviceConfig(uint32_t configAddress, uint8_t base, uint8_t size, void *dest)
{
    (void)configAddress; (void)base; (void)size; (void)dest;
    return 0;
}

int PCI_findOfType(int class, int subclass, int progif, int max, PCI_CommonDeviceHeader *buf)
{
    if (!buf) return -1;
    int matched = 0;
    PCI_CommonDeviceHeader tmp;
    for (int bus = 0; bus < 256; bus++)
    for (int device_slot = 0; device_slot < 32; device_slot++)
    for (int func = 0; func < 8; func++)
    {   
        ((uint32_t *)&tmp)[2] = pci_readConfigRegister(bus, device_slot, func, 8);
        if (
            (tmp.class_code == class || class < 0) &&
            (tmp.class_code == class || subclass < 0) &&
            (tmp.class_code == class || progif < 0)
        ) {
            matched++;
            if (matched <= max) {
                ((uint32_t *)&tmp)[0] = pci_readConfigRegister(bus, device_slot, func, 0);
                ((uint32_t *)&tmp)[1] = pci_readConfigRegister(bus, device_slot, func, 4);
                ((uint32_t *)&tmp)[3] = pci_readConfigRegister(bus, device_slot, func, 12);
                buf[matched - 1] = tmp;
            }
        }
    }
    return matched;
}
