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
    call    fillScreen

    jmp $                           ; Infinite loop to keep the program running


parseMultibootInfo:
    ; Multiboot2 information address is passed in ebx
    ; ebx: pointer to the multiboot2 information structure
    mov     esi,    ebx                 ; Save the pointer
    mov     eax,    [ebx]               ; Get the size
    add     esi,    eax                 ; Get the end of the structure
    add     ebx,    4                   ; Skip the reserved dword
    .checkTag:
        cmp     dword [ebx],  8         ; Check if the info is the framebuffer info
        je      .framebufferSetup

        mov     ecx,    dword [ebx]     ; Get the size of the current info
        add     ebx,    ecx             ; Skip the current info
        add     ebx,    7
        and     ebx,    -8

        cmp     ebx,    esi             ; Compare the current pointer to the end
        jge     .framebufferNotFound    ; If the frame buffer is not found

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

putPixel:
    ; Put a pixel
    ; Args:
    ;   esi: x position (col)
    ;   edi: y position (row)
    xor     eax,    eax
    mov     eax,    esi
    mov     cl,     byte [framebufBPP]
    shr     cl,     3
    mul     cl
    mov     esi,    eax

    xor     eax,    eax
    mov     eax,    dword [framebufPitch]
    mul     edi

    lea     eax,    [eax + esi]
    add     eax,    dword [framebuffer]

    mov     dword [eax], 0xFFFFFFFF

    ret

fillScreen:
    mov     esi,    0               ; X position
    mov     edi,    0               ; Y position
    mov     eax,    [framebuffer]   ; Start of framebuffer
    mov     ecx,    [framebufHeight]
.fillRow:
    push    ecx
    mov     ecx,    [framebufWidth]
.fillColumn:
    mov     dword [eax], 0x00FFFF00 ; White pixel (ARGB format)
    add     eax,    4               ; Move to the next pixel (4 bytes per pixel)
    loop    .fillColumn
    pop     ecx
    add     eax,    [framebufPitch] ; Move to the next row
    mov     ebx,    [framebufWidth] ; Load framebufWidth
    shl     ebx,    2               ; Multiply framebufWidth by 4 (shift left by 2)
    sub     eax,    ebx             ; Reset to the start of the next row
    loop    .fillRow
    ret
