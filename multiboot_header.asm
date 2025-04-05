[bits 32]
section .multiboot
    align 8
    dd 0xE85250D6               ; Multiboot2 magic number
    dd 0                        ; Architecture (0 for i386)
    dd 0x28                     ; Total length of the Multiboot2 header
    dd -(0xE85250D6 + 0 + 0x28) ; Checksum

    align 8
    header_tag_framebuffer:
        dd 4                    ; Type (Framebuffer tag)
        dd 16                   ; Size of this tag
        dw 640                  ; Width
        dw 480                  ; Height
        dw 32                   ; Bits per pixel
        dw 0                    ; Reserved

    align 8
    header_tag_end:
        dd 0                    ; Type (End tag)
        dd 8                    ; Size of this tag