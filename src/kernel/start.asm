extern kmain

stack:
    times 2048 dq 0
stack_top:

section .text

global _start
_start:
    lea     rsp,    [rel stack_top]
    xor     rbp,    rbp
    call    kmain

section .note.GNU-stack noexec
