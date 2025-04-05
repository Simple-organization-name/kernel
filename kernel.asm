[bits 64]

section .bss
    align 8
    framebuffer     resq 1
    framebufPitch   resd 1  ; nb of bytes in each row
    framebufWidth   resd 1
    framebufHeight  resd 1
    framebufBPP     resb 1  ; nb of bits per pixel
    fbRedPos        resb 1
    fbGreenPos      resb 1
    fbBluePos       resb 1

section .data:

section .text
    global __START__
__START__:
    call    parseMultibootInfo      ; Get the information from GRUB

    mov     rsi,    100
    mov     rdi,    100
    call    putPixel                ; Call the putPixel function

    jmp $                           ; Infinite loop to keep the program running


parseMultibootInfo:
    ; Multiboot2 information address is passed in ebx
    ; ebx: pointer to the multiboot2 information structure
    mov     esi,    ebx         ; Save the pointer
    mov     eax,    [ebx]       ; Get the size
    add     esi,    eax         ; Get the end of the structure
    add     ebx,    4           ; Skip the reserved dword
    .checkTag:
        cmp     dword [ebx],  8 ; Check if the info is the framebuffer info
        je      .framebufferSetup

        mov     ecx,    dword [ebx]   ; Get the size of the current info
        add     ebx,    ecx     ; Skip the current info

        cmp     ebx,    esi     ; Compare the current pointer to the end
        jge     .framebufferNotFound    ; If the frame buffer is not found

        jmp     .checkTag

    .framebufferNotFound:
        hlt

    .framebufferSetup:
        mov     r8,     qword [ebx + 8]
        mov     [framebuffer],      r8
        mov     r8d,     dword [ebx + 16]
        mov     [framebufPitch],    r8d
        mov     r8d,     dword [ebx + 20]
        mov     [framebufWidth],    r8d
        mov     r8d,     dword [ebx + 24]
        mov     [framebufHeight],   r8d
        mov     r8b,     byte [ebx + 28]
        mov     [framebufBPP],      r8b
        mov     r8b,     byte [ebx + 31]
        mov     [fbRedPos],         r8b
        mov     r8b,     byte [ebx + 33]
        mov     [fbGreenPos],       r8b
        mov     r8b,     byte [ebx + 35]
        mov     [fbBluePos],        r8b
        ret

putPixel:
    ; Put a pixel
    ; Args:
    ;   rsi: x position (col)
    ;   rdi: y position (row)
    xor     rax,    rax
    mov     rax,    rsi
    mov     cl,     byte [framebufBPP]
    mul     cl
    mov     rsi,    rax

    mov     rax,    rdi
    mov     cl,     byte [framebufPitch]
    mul     cl
    
    lea     rax,    [rax + rsi]
    add     rax,    qword [framebuffer]

    mov     dword [rax], 0xFFFFFFFF

    ret

