#ifndef __IDT_H__
#define __IDT_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct _register_state {
    uint64_t rax, rbx, rcx, rdx, rdi, rsi, r8, r9, r10, r11, r12, r13, r14, r15;
} register_state_t;

typedef struct _interrupt_frame {
    register_state_t registers;
    uint64_t int_no;
    uint64_t err_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    struct {
        uint64_t rsp;
        uint64_t ss;
    } on_cpl;
} interrupt_frame_t;

void set_interrupt(uint8_t int_no, void *func, bool trap);
void init_interrupts();
void interrupt_handler(interrupt_frame_t *context);

#endif
