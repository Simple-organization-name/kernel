
section .text
    extern interrupt_handler

%macro push_regs 0
    ; push rbp
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rsi
    push rdi
    push rdx
    push rcx
    push rbx
    push rax
%endmacro
%macro pop_regs 0
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rdi
    pop rsi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    ; pop rbp
%endmacro


isr_base:
    push_regs
    mov     rdi,    rsp
    ; xor rdx, rdx
    ; mov rax, rsp
    ; mov rcx, 16
    ; div rcx
    ; push rdx
    ; sub rsp, rdx
    ; mov     r12,    rsp
    ; and     rsp,    ~0x8
    call    interrupt_handler
    ; pop rdx
    ; add rsp, rdx
    ; mov     rsp,    r12
    pop_regs
    add     rsp,    16  ; pop int number and error code
    ; hlt
    iretq

%macro build_isr_err 1
isr_no_%1:
    push    %1
    jmp     isr_base
%endmacro
%macro build_isr_noerr 1
isr_no_%1:
    push    0   ; standardise stack layout by putting dummy error code
    push    %1
    jmp     isr_base
%endmacro

; cpu exceptions
; ref at https://wiki.osdev.org/Exceptions for err / noerr
build_isr_noerr 0
build_isr_noerr 1
build_isr_noerr 2
build_isr_noerr 3
build_isr_noerr 4
build_isr_noerr 5
build_isr_noerr 6
build_isr_noerr 7
build_isr_err 8
build_isr_noerr 9
build_isr_err 10
build_isr_err 11
build_isr_err 12
build_isr_err 13
build_isr_err 14
build_isr_noerr 15
build_isr_noerr 16
build_isr_err 17
build_isr_noerr 18
build_isr_noerr 19
build_isr_noerr 20
build_isr_err 21
build_isr_noerr 22
build_isr_noerr 23
build_isr_noerr 24
build_isr_noerr 25
build_isr_noerr 26
build_isr_noerr 27
build_isr_noerr 28
build_isr_err 29
build_isr_err 30
build_isr_noerr 31
; PIC interrupts (all noerr)
build_isr_noerr 32
build_isr_noerr 33
build_isr_noerr 34
build_isr_noerr 35
build_isr_noerr 36
build_isr_noerr 37
build_isr_noerr 38
build_isr_noerr 39
build_isr_noerr 40
build_isr_noerr 41
build_isr_noerr 42
build_isr_noerr 43
build_isr_noerr 44
build_isr_noerr 45
build_isr_noerr 46
build_isr_noerr 47

section .data
global isr_stub_table
isr_stub_table:
    %assign i 0
    %rep 48
    dq isr_no_%+i
    %assign i i+1
    %endrep

section .note.GNU-stack noexec
