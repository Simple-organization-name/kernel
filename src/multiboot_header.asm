[bits 32]
section .multiboot2_header
    align 8
    dd 0xE85250D6               ; Multiboot2 magic number
    dd 0                        ; Architecture
    dd headerEnd - headerStart  ; Total length of the Multiboot2 header
    dd -(0xE85250D6 + 0 + (headerEnd - headerStart))        ; Checksum

    align 8
headerStart:
    bufferframeTag:
        dw 5    ; Tag type 5 = framebuffer
        dw 1    ; Flags (0 = optionnal, 1 = required)
        dd 20   ; Size of the tag
        dd 640  ; Width
        dd 480  ; Height
        dd 32   ; Depth

    align 8
headerEnd:
    dd 0                    ; Type (End tag)
    dd 8                    ; Size of this tag
    ; Multiboot2 header end