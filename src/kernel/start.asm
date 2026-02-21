extern kmain

stack:
    times 2096 dq 0
stack_top:

section .text

global _start
_start:
    lea     rbp,    [rel stack_top]
    mov     rsp,    rbp
    jmp     kmain

section .note.GNU-stack noexec
