extern kmain

section .text
stack:
    times 2048 dq 0
stack_top:

global _start
_start:
    lea     rsp,    [rel stack_top]
    jmp     kmain

section .note.GNU-stack noexec
