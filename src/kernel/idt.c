#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <idt.h>
#include <kterm.h>

#include <asm.h>

typedef struct _idtr {
    uint16_t size_minus_one;
    uint64_t addr;
} __attribute__((__packed__)) idtr_t;

typedef struct _idt_entry {
    uint16_t offset_0;
    uint16_t segment;
    uint8_t ist;
    union {
        uint8_t flags;
        struct {
                uint8_t gate_type : 4,
                    zero : 1,
                    dpl : 2,
                    present : 1;
        };
    };
    uint16_t offset_1;
    uint32_t offset_2;
    uint32_t reserved;
} __attribute__((__packed__)) idt_entry_t;

#define NUM_IDTE 256

static idt_entry_t _IDT[NUM_IDTE] __attribute__((aligned(4096)));

#define PIC_MASTER_COMMAND    0x20
#define PIC_MASTER_DATA       0x21
#define PIC_SLAVE_COMMAND    0xA0
#define PIC_SLAVE_DATA       0xA1

#define ICW1_INIT       0x10
#define ICW1_ICW4       0x01
#define ICW4_8086        0x01

#define PIC_IRQ_OFFSET  32

void set_interrupt(uint8_t int_no, void* func, bool trap)
{
    // if int is in a PIC, disable the mask
    // if (int_no >= PIC_IRQ_OFFSET && int_no < PIC_IRQ_OFFSET + 8) {
    //     outb(PIC_MASTER_DATA, inb(PIC_MASTER_DATA) & ~(uint8_t)(1 << (int_no - PIC_IRQ_OFFSET)));
    // } else if (int_no >= PIC_IRQ_OFFSET + 8 && int_no < PIC_IRQ_OFFSET + 16) {
    //     outb(PIC_SLAVE_DATA, inb(PIC_SLAVE_DATA) & ~(uint8_t)(1 << (int_no - (PIC_IRQ_OFFSET + 8))));
    // }
    _IDT[int_no] = (idt_entry_t){
        .offset_0 = (uint64_t)func & 0xFFFF,
        .segment = 0x08,
        .ist = 0,
        .gate_type = trap ? 0xF : 0xE,  // Use struct fields instead of manual bit manipulation
        .zero = 0,
        .dpl = 0,
        .present = 1,
        .offset_1 = ((uint64_t)func >> 16) & 0xFFFF,
        .offset_2 = ((uint64_t)func >> 32) & 0xFFFFFFFF,
        .reserved = 0
    };
}

extern void* isr_stub_table[];
void init_interrupts()
{
    _Static_assert(NUM_IDTE >= 1 && NUM_IDTE <= 256);
    _Static_assert(sizeof(idtr_t) == 10);

    for (uint32_t i = 0; i < NUM_IDTE; i++)
    {
        _IDT[i].present = 0;
    }


    idtr_t idtr = (idtr_t){
        .size_minus_one = (sizeof _IDT) - 1,
        .addr = (uint64_t)_IDT
    };


    // save which interrupts are enabled to set them back later just in case
    // uint8_t master_mask = inb(PIC_MASTER_DATA);
    // uint8_t slave_mask = inb(PIC_SLAVE_DATA);

    // [ICW1]
    // start PIC init and enable fourth init step
    outb(PIC_MASTER_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC_SLAVE_COMMAND, ICW1_INIT | ICW1_ICW4);

    // [ICW2]
    // set irq base for each chip
    outb(PIC_MASTER_DATA, PIC_IRQ_OFFSET); // set master after 32 first to not clash with cpu interrupts
    outb(PIC_SLAVE_DATA, PIC_IRQ_OFFSET + 8); // set slave right after master

    // [ICW3]
    // slave is wired to master's IR2, gotta notify the master with a bitmask and the slave with an id
    outb(PIC_MASTER_DATA, 1 << 2);
    outb(PIC_SLAVE_DATA, 2);

    // [ICW4]
    // we tell the thing to behave like a normal pc and not some dinosaur of a machine like the 8080
    outb(PIC_MASTER_DATA, ICW4_8086);
    outb(PIC_SLAVE_DATA, ICW4_8086);

    // // we put back the mask, can change it later anyways
    // i'm silly so i disabled everything
    outb(PIC_MASTER_DATA, 0xFD);
    outb(PIC_SLAVE_DATA, 0xFF);

    for (uint32_t i = 0; i < PIC_IRQ_OFFSET + 16; i++)
    {
        set_interrupt(i, isr_stub_table[i], true);
    }


    __asm__ volatile("lidt %0" :: "m"(idtr) : "memory");
    __asm__ volatile ("sti" ::: "memory");
}

void interrupt_handler(interrupt_frame_t* context)
{
    if (context->int_no >= PIC_IRQ_OFFSET && context->int_no < PIC_IRQ_OFFSET + 16) {
        if (context->int_no >= PIC_IRQ_OFFSET + 8) {
            outb(PIC_SLAVE_COMMAND, 0x20);
        }
        outb(PIC_MASTER_COMMAND, 0x20);
    }

    switch (context->int_no)
    {
    case 0x00:  // division error
        kprintf("A division error has occured at RIP=0x%X.\n", context->rip);
        hlt();
        return;
    
    case 0x06:  // invalid opcode
        kprintf("An invalid opcode was encountered at RIP=0x%X.\n", context->rip);
        hlt();
        return;

    case 0x07:
        kprintf("Tried to execute an FPU instruction while it was absent or disabled at RIP=0x%X.\n", context->rip);
        hlt();
        return;

    case 0x08:  // double fault
        kprintf("A double fault has occured at RIP=0x%X.\n", context->rip);
        kputs("Execution will be frozen to prevent an automatic reboot.\n");
        cli();
        while (1) hlt();

    case 0x0D:  // general protection fault
        kprintf("A general protection fault was triggered at RIP=0x%x.\n", context->rip);
        if (context->err_code != 0) {
            kprintf("It was related to segment selector number %X.\n", context->err_code);
        }
        hlt();
        return;

    case 0x0E:  // page fault
        uint64_t addr;
        __asm__ volatile (
            "mov %%cr2, %0"
            : "=r"(addr)
            :: "memory"
        );
        kprintf("\nPage fault at address 0x%X, caused by a %s access during %s.\n", addr, context->err_code & 2 ? "write" : "read", context->err_code & 32 ? "an instruction fetch" : "a memory access");
        kprintf("Caused by a %s\n", context->err_code & 1 ? "page protection violation" : "non-present page");
        kprintf("Caused at RIP=0x%X, in %s mode.\n", context->rip, context->err_code & 4 ? "user" : "kernel");
        kputs("\n  ---==== REGISTERS ====---\n");
        kprintf("RAX = 0x%X | RBX = 0x%X\n", context->registers.rax, context->registers.rbx);
        kprintf("RCX = 0x%X | RDX = 0x%X\n", context->registers.rcx, context->registers.rdx);
        kprintf("RDI = 0x%X | RSI = 0x%X\n", context->registers.rdi, context->registers.rsi);
        kprintf("R8 = 0x%X | R9 = 0x%X\n", context->registers.r8, context->registers.r9);
        kprintf("R10 = 0x%X | R11 = 0x%X\n", context->registers.r10, context->registers.r11);
        kprintf("R12 = 0x%X | R13 = 0x%X\n", context->registers.r12, context->registers.r13);
        kprintf("R14 = 0x%X | R15 = 0x%X\n", context->registers.r14, context->registers.r15);
        kputs("\n  ---==== CODE DUMP ====---\n");
        if (context->err_code & 32) {
            kputs("Error due to code fetch; will not fetch code\n");
        } else {
            for (unsigned i = 0; i < 20; i++) {
                kprintf("%x ", ((uint8_t *)context->rip)[i]);
            }
        }
        
        hlt();
        return;

    case 0x21:  // PS/2 keyboard IRQ
        uint8_t chr = inb(0x60);
        if (!(chr & 0x80)) kputc(chr);
        return;
    
    default:
        kprintf("Interrupt vector %X was triggered but is not handled.\n", context->int_no);
        return;
    }
}