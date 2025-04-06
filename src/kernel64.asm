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

section .data

section .text
    global __START__
__START__:
    ; Set up the stack
    mov     rsp,    0x100000
    mov     rbp,    rsp

    call    parseMultibootInfo      ; Get the information from GRUB

    ; Test framebuffer
    mov     rax,    qword [framebuffer]
    test    rax,    rax             ; Check if framebuffer address is non-zero
    jz      .framebufferError       ; Halt if framebuffer is not initialized

    mov     dword [rax],    0xFFFFFFFF

.framebufferError:
__END__:
    hlt

parseMultibootInfo:
    ; Multiboot2 information address is passed in ebx
    ; ebx: pointer to the multiboot2 information structure
    mov     esi,    ebx                 ; Save the pointer
    mov     eax,    [ebx]               ; Get the size
    add     esi,    eax                 ; Get the end of the structure
    add     ebx,    8                   ; Skip the size dword and the reserved dword
    .checkTag:
        cmp     dword [ebx],  0         ; Check if the info is the end of the structure
        je      .framebufferNotFound
        cmp     dword [ebx],  8         ; Check if the info is the framebuffer info
        je      .framebufferSetup

        mov     ecx,    dword [ebx + 4] ; Get the size of the current info
        add     ebx,    ecx             ; Skip the current info

        add     ebx,    7
        and     ebx,    -8

        cmp     ebx,    esi             ; Compare the current pointer to the end
        jge     .framebufferNotFound    ; If the frame buffer is not found

        jmp     .checkTag

    .framebufferNotFound:
        hlt

    .framebufferSetup:
        mov     rax,    qword [ebx + 8]
        mov     [framebuffer],      rax
        mov     eax,    dword [ebx + 16]
        mov     [framebufPitch],    eax
        mov     eax,    dword [ebx + 20]
        mov     [framebufWidth],    eax
        mov     eax,    dword [ebx + 24]
        mov     [framebufHeight],   eax
        mov     al,     byte [ebx + 28]
        mov     [framebufBPP],      al
        mov     al,     byte [ebx + 31]
        mov     [fbRedPos],         al
        mov     al,     byte [ebx + 33]
        mov     [fbGreenPos],       al
        mov     al,     byte [ebx + 35]
        mov     [fbBluePos],        al
        ret
