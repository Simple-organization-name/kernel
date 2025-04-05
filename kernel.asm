[bits 32]

section .bss
    framebuffer resd 1       ; Framebuffer address
    fb_width    resd 1       ; Framebuffer width
    fb_height   resd 1       ; Framebuffer height
    fb_pitch    resd 1       ; Framebuffer pitch (bytes per row)
    fb_bpp      resd 1       ; Bits per pixel

section .data:
    .testString db 'HELLO', 0

section .text
    global __START__
__START__:
    cli
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

    mov eax, 1
    mov ebx, 1
    mov ecx, 0xFF0000       ; Red color
    call putPixel           ; Put pixel at (1, 1) with red color

    mov eax, 1
    mov ebx, 2
    mov ecx, 0x00FF00       ; Green color
    call putPixel           ; Put pixel at (2, 2) with green color

    mov eax, 1
    mov ebx, 3
    mov ecx, 0x0000FF       ; Blue color
    call putPixel           ; Put pixel at (3, 3) with blue color

    hlt


putPixel:
    ; Put a pixel on the screen
    ; Args:
    ;   eax: X coordinate
    ;   ebx: Y coordinate
    ;   ecx: Color (RGB)
    push ebp
    mov ebp, esp

    mov edx, [framebuffer] ; Load framebuffer address
    mov esi, [fb_pitch]    ; Load pitch (bytes per row)
    mov edi, [fb_bpp]      ; Load bits per pixel

    ; Calculate pixel address
    mul esi                ; ebx * pitch (y * pitch)
    add edx, eax           ; Add framebuffer base + y * pitch
    shr edi, 3             ; Divide bpp by 8 (bytes per pixel)
    mul edi                ; eax = x * (bpp / 8)
    add edx, eax           ; Add x offset to framebuffer address

    ; Write the pixel
    mov [edx], ecx         ; Write the color to the calculated address

    pop ebp
    ret