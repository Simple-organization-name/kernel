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

typedef struct __attribute__((aligned(16))) _uhci_td {
    UHCI_Pointer linkPointer;
    union {
        uint32_t whole;
        struct {
            uint8_t __r1: 2,
                    shortPacketDetect: 1,
                    errorLimit: 2,  // 0 for no error limit, or max 3 for max error count allowed. see docs for accounted errors.
                    lowSpeedDevice: 1,  // target device is low speed (1.5Mb/s). see token for device
                    isochronous: 1, // welp like read the thing (btw isochronous are marked inactive no matter the errors)
                    interruptOnFrameEnd: 1; // wether to issue interrupt when frame TD is in is completed
            union {
                uint8_t whole;
                struct {
                    uint8_t active: 1,  // set to 1 to execute, written 0 on completion or stall
                            stalled: 1, // stall happens when error or device just says welp yes but frick you
                            dataBufferError: 1, // something like speeds not matching (overrun/underrun) -> cause of actLen != maxLen
                            babbleDetected: 1,  // refer to bandwidth reclamation packets
                            NAK_Received: 1,    // negative acknowledgement => should mean "not ready" ?
                            CRCerr_or_timeOut: 1,   // pretty staightforward
                            bitstuffError: 1,   // data stream contained more than 6 0b1 in a row, refer to further protocol docs ig
                            __r: 1;
                };
            } status;
            uint16_t __r2: 5,
                    actLen: 11; // must be set to 0 at start, written to on complete
        };
    } controlAndStatus;
    union {
        uint32_t whole;
        struct {
            uint32_t maxLen: 11,    // put n-1 in here, max 0x4FF = 1280 bytes, magic value 0x7FF for Null Data packet
                    __r: 1,
                    dataToggle: 1,
                    endPoint: 4,    // device endpoint selector
                    deviceAddress: 7,   // device selector
                    packetID: 8;    // IN = 0x69, OUT = 0xE1, SETUP = 0x2D, other values are invalid
        };
    } token;
    uint32_t bufferPointer; // physical address of transaction data buffer, of size maxLen and arbitrary alignment
    uint32_t forYou[4]; // ig do what we want here, maybe put info for our own traversal ? or discard entirely since TD's are 16b-aligned
} UHCI_TransferDescriptor;

typedef struct _uhci_qh {
    UHCI_Pointer afterQueue;
    UHCI_Pointer queueElement;
} UHCI_QueueHead;

void init_UHCI();

#endif
