[bits 32]

section .bss
    align 8
    framebuffer     resd 1
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
    mov     esp,    0x100000
    mov     ebp,    esp

    call    parseMultibootInfo      ; Get the information from GRUB

    ; Test framebuffer
    mov     eax,    dword [framebuffer]
    mov     dword [eax + 64],    0xFFFFFFFF

    hlt                             ; Infinite loop to keep the program running


parseMultibootInfo:
    ; Multiboot2 information address is passed in ebx
    ; ebx: pointer to the multiboot2 information structure
    add     ebx,    8                   ; Skip the reserved dword
    .checkTag:
        cmp     dword [ebx],  0         ; Check if the info is the end of the structure
        je      .framebufferNotFound
        cmp     dword [ebx],  8         ; Check if the info is the framebuffer info
        je      .framebufferSetup

        mov     ecx,    dword [ebx]     ; Get the size of the current info
        add     ebx,    ecx             ; Skip the current info
        add     ebx,    7
        and     ebx,    -8

        jmp     .checkTag

    .framebufferNotFound:
        hlt

    .framebufferSetup:
        mov     eax,    dword [ebx + 8]
        mov     [framebuffer],      eax
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
