#include <UHCI.h>
#include <stddef.h>
#include <kterm.h>
#include <kalloc.h>

#include <PCI.h>


// https://ftp.netbsd.org/pub/NetBSD/misc/blymn/uhci11d.pdf

static UHCI_FrameList frameList;

typedef char nothing[0];

void init_UHCI()
{
    PCI_CommonDeviceHeader* buf = (PCI_CommonDeviceHeader *)kallocPage(MEM_4K);
    int max = 4096 / sizeof(PCI_CommonDeviceHeader);
    
    int matched = PCI_findOfType(UHCI_PCI_CLASS_ID, UHCI_PCI_SUBCLASS_ID, UHCI_PCI_INTERFACE, max, buf);
    matched = matched > max ? max : matched;    // truncate, dont care about potentially getting more than 256 usb1 devices

    frameList = kallocPage(MEM_4K);
    for (int i = 0; i < 1024; i++)
    {
        frameList[i] = UHCI_POINTER_TERMINATE;
    }
    
    if (!matched) {
        kputs("No UHCI devices found lmao\n");
    } else if (matched >= 2) {
        kprintf("I see you got %d UHCI devices there but i'm only gonna do one init :)\n", matched);
    }

    
    
}
