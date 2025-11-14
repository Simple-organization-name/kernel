#ifndef __UHCI_H__
#define __UHCI_H__

#include <stdint.h>

#define UHCI_PCI_CLASS_ID       0x0C
#define UHCI_PCI_SUBCLASS_ID    0x03
#define UHCI_PCI_INTERFACE      0x00

typedef struct _uhci_io_reg {
    uint16_t USBCMD;
    uint16_t USBSTS;
    uint16_t USBINTR;
    uint16_t FRNUM;
    uint32_t FRBASEADD;
    uint8_t  SOFMOD;
    uint8_t  __padding[3];
    uint16_t PORTSC1;
    uint16_t PORTSC2;
} UHCI_IO_registers;

void init_UHCI();

#endif
