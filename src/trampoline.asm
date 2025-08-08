
[bits 64]

section .rodata
    align 8
gdt:
    ; only the access byte changes:
    ;   Dpl = kernel ? 0 : 3
    ;   E = code ? 1 : 0
    .nullDesc dq 0
    .kernelCode:
        dw 0xFFFF ; limit
        dw 0x0000 ; base
        db 0x00 ; base
        db 0b10011010 ; access byte
        ;    P  SEDcA 
        ;     Dpl  RW
        db 0b10100000 ; flags + limit
        ;    G
        db 0x00 ; base
    .kernelData:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0b10010010
        db 0b10100000
        db 0x00
    .userCode:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0b11111010
        db 0b10100000
        db 0x00
    .userData:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0b11110010
        db 0b10100000
        db 0x00
gdt_end:

section .data
    align 8
gdtr:
    dw 0
    dq 0

section .bss
    global kstack_base
    global kstack_top

    align 8
    kstack_base resb 0x10000
    kstack_top:

section .text
    extern kernelMain

    global _start

    align 8
_start:
; set stack start
    lea     rbp,    [rel kstack_top]
    mov     rsp,    rbp
; set stack end

    push    rcx ; push the pointer of kernel info
    cli

; set gdt start
    lea     rax,    [rel gdt_end]
    lea     rbx,    [rel gdt]
    mov     qword [rel gdtr + 2], rbx
    sub     rax,    rbx
    sub     rax,    1
    mov     word [rel gdtr], ax

    lgdt    [rel gdtr] ; Set the gdt
; set gdt end

; paging start
    align 8
    
; paging end

; reload segment registers
    push    0x08
    lea     rax,    [rel .reload_cs]
    push    rax
    retfq ; 64bit retf

.reload_cs:
    mov     ax,     0x10
    mov     ds,     ax
    mov     es,     ax
    mov     fs,     ax
    mov     gs,     ax
    mov     ss,     ax
; end reload

    align 8
    sti
    pop     rdi
    call    kernelMain

; Mark stack as non-executable
section .note.GNU-stack noexec
