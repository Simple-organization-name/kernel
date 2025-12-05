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

typedef uint32_t UHCI_Pointer;
#define UHCI_POINTER_TERMINATE  (1 << 0)
#define UHCI_POINTER_QUEUE      (1 << 1)
#define UHCI_POINTER_DEPTH      (1 << 2)
#define UHCI_POINTER_PTR        (0xFFFFFFF0)
typedef UHCI_Pointer* UHCI_FrameList;

typedef struct _uhci_td {
    UHCI_Pointer linkPointer;
    union {
        uint32_t whole;
        struct {
            uint8_t __r1: 2,
                    shortPacketDetect: 1,
                    errorLimit: 2,
                    lowSpeedDevice: 1,
                    isochronous: 1,
                    interruptOnFrameEnd: 1;
            union {
                uint8_t whole;
                struct {
                    uint8_t active: 1,
                            stalled: 1,
                            dataBufferError: 1,
                            babbleDetected: 1,
                            NAK_Received: 1,
                            CRCerr_or_timeOut: 1,
                            bitstuffError: 1,
                            __r: 1;
                };
            } status;
            uint16_t __r2: 5,
                    actLen: 11;
        };
    } controlAndStatus;
    union {
        uint32_t whole;
        struct {
            uint32_t maxLen: 11,
                    __r: 1,
                    dataToggle: 1,
                    endPoint: 4,
                    deviceAddress: 7,
                    packetID: 8;
        };
    } token;
    uint32_t bufferPointer;
    uint32_t forYou[4];
} UHCI_TransferDescriptor;

typedef struct _uhci_qh {
    UHCI_Pointer afterQueue;
    UHCI_Pointer queueElement;
} UHCI_QueueHead;

void init_UHCI();

#endif
