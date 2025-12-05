#include <UHCI.h>
#include <stddef.h>
#include <kterm.h>
#include <kalloc.h>

#include <PCI.h>


// https://ftp.netbsd.org/pub/NetBSD/misc/blymn/uhci11d.pdf

void init_UHCI()
{
    UHCI_FrameList frameList = (UHCI_FrameList)kallocPage(MEM_4K);
    (void)frameList;
    UHCI_TransferDescriptor td;
}
