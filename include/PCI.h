#ifndef __PCI_H__
#define __PCI_H__

#include <stdint.h>

#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

#define PCI_CONSTRUCT_CONFIG_ADDRESS(bus, device, function, regoff) \
    (1U << 31) | \
    ((bus & 0xFF) << 16) | \
    ((device & 0x1F) << 11) | \
    ((function & 0x7) << 8) | \
    (regoff & 0xFC)

struct _pci_common_header {
    uint16_t    vendor_id;      // 0xFFFF is invalid, look at pcisig/member-companies to get a list
    uint16_t    device_id;      // id allocated by vendor so idk
    uint16_t    command;        // they said idk but sending a 0 disconnects it
    uint16_t    status;         // PCI bus events
    uint8_t     revision_id;    // allocated by the vendor per device so idk
    const uint8_t prog_if;        // 
    const uint8_t subclass;
    const uint8_t class_code;
    uint8_t     cache_line_size;
    uint8_t     latency_timer;
    uint8_t     header_type;
    uint8_t     bist;
};

void printAllPCI();

#endif
