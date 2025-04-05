[bits 32]

section .bss
    framebuffer resd 1       ; Framebuffer address
    fb_width    resd 1       ; Framebuffer width
    fb_height   resd 1       ; Framebuffer height
    fb_pitch    resd 1       ; Framebuffer pitch (bytes per row)
    fb_bpp      resd 1       ; Bits per pixel

section .data:

section .text
    global __START__
__START__:
    mov eax, [ebx + 12]     ; Framebuffer address
    mov [framebuffer], eax

    mov eax, [ebx + 16]     ; Framebuffer pitch (bytes per row)
    mov [fb_pitch], eax

    mov eax, [ebx + 20]     ; Framebuffer width
    mov [fb_width], eax

    mov eax, [ebx + 24]     ; Framebuffer height
    mov [fb_height], eax

    mov eax, [ebx + 28]     ; Bits per pixel
    mov [fb_bpp], eax

    mov eax, 50                ; X coordinate
    mov ebx, 50                ; Y coordinate
    mov ecx, 0xFFFF0000        ; Red color
    call putPixel              ; Call putPixel to draw the pixel

    jmp $                   ; Infinite loop to keep the program running


putPixel:
    ; Put a pixel on the screen
    ; Args:
    ;   eax: X coordinate
    ;   ebx: Y coordinate
    ;   ecx: Color (ARGB)
    push ebp
    mov ebp, esp

    push ebx                ; Save Y coordinate
    push eax                ; Save X coordinate

    mov edx, [framebuffer]  ; Load framebuffer address
    mov esi, [fb_pitch]     ; Load pitch (bytes per row)
    mov edi, [fb_bpp]       ; Load bits per pixel

    ; Calculate Y offset
    mov eax, ebx            ; Copy Y coordinate to eax
    mul esi                 ; eax = y * pitch
    add edx, eax            ; Add Y offset to framebuffer address

    ; Calculate X offset
    pop eax                 ; Restore X coordinate to eax
    shr edi, 3              ; Divide bpp by 8 (bytes per pixel)
    mul edi                 ; eax = x * (bpp / 8)
    add edx, eax            ; Add X offset to framebuffer address

    ; Write the pixel
    mov [edx], ecx          ; Write the color to the calculated address

    pop ebx                 ; Restore Y coordinate

    pop ebp
    ret

