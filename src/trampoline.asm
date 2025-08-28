[bits 64]
%define fillSize 5

;; called from Win32PE x64 ABI : rcx, rdx, r8, r9, stack

; here assumed without BootInfo* cuz ughh so _start(pml4)

_start:
    mov     r10,    rcx ; save pml4
    mov     r11,    rdx ; save bootinfo
    mov     r12,    r8  ; save kernel entry point

    ; disable interrupts and let kernel put them back on
    cli

    ; ensure long mode and paging bits and stuff
    mov     rcx,    0xC0000080 ; IA32_EFER
    rdmsr
    or      eax,    ((1<<8) | (1 << 11))  ; IA32_EFER.LME (long mode enable) and IA32_EFER.NXE (noexec page table bit enable)
    wrmsr

    mov     rax,    cr4
    or      rax,    ((1<<5) | (1<<9) | (1<<10)) ; CR4.PAE (needed for long mode paging) and simd/XMM stuff (pretty neat if you ask me)
    mov rbx, (1<<17)  | (1 << 23)
    not rbx
    and rax, rbx
    mov     cr4,    rax

    mov     rax,    cr0
    mov     rbx,    1
    shl     rbx,    16  ; write protect
    or      rax,    rbx
    mov     rbx,    1
    shl     rbx,    31  ; paging
    or      rax,    rbx
    mov     cr0,    rax

    ; patch gdt and set it
    lea     rax,    [rel gdt_end]
    lea     rbx,    [rel gdt]
    mov     qword [rel gdtr + 2],   rbx
    sub     rax,    rbx
    sub     rax,    1
    mov     word [rel gdtr],    ax
    lgdt    [rel gdtr]

    ; reload segment descriptors
    lea     rsp,    [rel kstack_temp]
    push    0x08
    lea     rax,    [rel .reload_cs]
    push    rax
    retfq
.reload_cs:
    ; mov     ax,     0x10
    ; mov     ds,     ax
    ; mov     es,     ax
    ; mov     fs,     ax
    ; mov     gs,     ax
    ; mov     ss,     ax

    mov rbx, 0xFFF
    not rbx
    and r10, rbx
    mov     cr3,    r10

    ; mov rdi, 0xFFFFFFFF
    ; mov r8, 0
    ; mov r9, 0
    ; call fill

    mov rax, 0xFFFFFFFF
    mov rbx, 0xFFFF0000
    cmp byte [r12], 0x49
    cmovne rbx, rax
    cmp byte [r12 + 1], 0x89
    cmovne rbx, rax
    cmp byte [r12 + 2], 0xFB
    cmovne rbx, rax

    mov     rax,    qword [r11] ; rax = &framebuffer
    mov     rdx,    qword [rax] ; rdx = fb.addr
    mov     rcx,    qword [rax + 8] ; rcx = fb.size
    add     rcx,    rdx ; rcx = fb.end
    .fill:
        cmp     rdx, rcx
        jge     .endfill
        mov     dword [rdx], ebx
        add     rdx, 4
        jmp     .fill
    .endfill:


; call kernel(bootinfo)
    mov     rdi,    r11
    jmp     r12

; section .rodata
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
        db 0b11000000
        db 0x00
    ; .userCode:
    ;     dw 0xFFFF
    ;     dw 0x0000
    ;     db 0x00
    ;     db 0b11111010
    ;     db 0b10100000
    ;     db 0x00
    ; .userData:
    ;     dw 0xFFFF
    ;     dw 0x0000
    ;     db 0x00
    ;     db 0b11110010
    ;     db 0b10100000
    ;     db 0x00
gdt_end:
    align 8
gdtr:
    dw 0
    dq 0

section .bss
    kstack_temp:
        resq 2048
    kstack_temp_top:
