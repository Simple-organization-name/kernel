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


typedef struct _pci_dev_hdr {
    uint16_t    vendor_id;
    uint16_t    device_id;
    uint16_t    command;
    uint16_t    status;
    uint8_t     revision_id;
    uint8_t     prog_if;
    uint8_t     subclass;
    uint8_t     class_code;
    uint8_t     cache_line_size;
    uint8_t     latency_timer;
    uint8_t     header_type;
    uint8_t     bist;
} PCI_CommonDeviceHeader;

typedef uint32_t pci_address_t;



void printAllPCI();

int getDeviceConfig(uint32_t configAddress, uint8_t base, uint8_t size, void* dest);

/**
 * @brief Enumerate PCI devices of a certain type.
 * @param class Desired device class. Negative for any.
 * @param subclass Desired device subclass. Negative for any.
 * @param progif Desired programming interface type. Negative for any.
 * @param max Maximum number of devices to be fetched.
 * @param buf Buffer to write matching devices to.
 * @returns The number of devices matched, or -1 if `buf == NULL`
 * 
 * If `buf` is `NULL`, 0 shall be returned.
 * If `ret <= max`, all PCI devices were scanned.
 * If `ret > max`, only `max` devices were written and caller shall re-scan to fetch the others.
 */
int PCI_findOfType(int class, int subclass, int progif, int max, PCI_CommonDeviceHeader* buf);

#define PCI_CLASS_UNCLASSIFIED                          0x0
#define PCI_CLASS_MASS_STORAGE_CONTROLLER               0x1
#define PCI_CLASS_NETWORK_CONTROLLER                    0x2
#define PCI_CLASS_DISPLAY_CONTROLLER                    0x3
#define PCI_CLASS_MULTIMEDIA_CONTROLLER                 0x4
#define PCI_CLASS_MEMORY_CONTROLLER                     0x5
#define PCI_CLASS_BRIDGE                                0x6
#define PCI_CLASS_SIMPLE_COMMUNICATION_CONTROLLER       0x7
#define PCI_CLASS_BASE_SYSTEM_PERIPHERAL                0x8
#define PCI_CLASS_INPUT_DEVICE_CONTROLLER               0x9
#define PCI_CLASS_DOCKING_STATION                       0xA
#define PCI_CLASS_PROCESSOR                             0xB
#define PCI_CLASS_SERIAL_BUS_CONTROLLER                 0xC
#define PCI_CLASS_WIRELESS_CONTROLLER                   0xD
#define PCI_CLASS_INTELLIGENT_CONTROLLER                0xE
#define PCI_CLASS_SATELLITE_COMMUNICATION_CONTROLLER    0xF
#define PCI_CLASS_ENCRYPTION_CONTROLLER                 0x10
#define PCI_CLASS_SIGNAL_PROCESSING_CONTROLLER          0x11
#define PCI_CLASS_PROCESSING_ACCELERATOR                0x12
#define PCI_CLASS_NON_ESSENTIAL_INSTRUMENTATION         0x13
#define PCI_CLASS_CO_PROCESSOR                          0x40
#define PCI_CLASS_VENDOR_SPECIFIC                       0xFF

#endif
