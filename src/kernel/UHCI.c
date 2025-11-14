#include <UHCI.h>
#include <stddef.h>
#include <kterm.h>

void init_UHCI()
{
    kprintf("Offset of PORTSC1 in UHCI io regs : %d\n", offsetof(UHCI_IO_registers, PORTSC1));
}

