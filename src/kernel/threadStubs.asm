%include "include/common.inc"

extern exit_thread

section .text
    global thread_init_stub
    global thread_switch_stub
    global thread_ret_stub

thread_init_stub:
    pushfq
    push_regs
    mov     qword [rsi],    rsp
    mov     rsp,    qword [rdi]
    mov     rbp,    rdi
    call    rdx
    call exit_thread
    jmp     $

thread_switch_stub:
    pushfq
    push_regs
    mov     qword [rdi],    rsp
    mov     rsp,    qword [rsi]

thread_ret_stub:
    pop_regs
    popfq
    ret
