[bits 32]
section .multiboot2_header
    align 8
    dd 0xE85250D6               ; Multiboot2 magic number
    dd 0                        ; Architecture
    dd headerEnd - headerStart  ; Total length of the Multiboot2 header
    dd -(0xE85250D6 + 0 + (headerEnd - headerStart))        ; Checksum

headerStart:
    align 8
    informationRequestTag:
        dw 1                    ; Tag type 1 = information request
        dw 0                    ; Flags (0 = optionnal, 1 = required)
        dd informationRequestTagEnd - informationRequestTag
        dd 8                    ; Request info 8 = framebuffer
    informationRequestTagEnd:
    align 8
    bufferframeTag:
        dw 5    ; Tag type 5 = framebuffer
        dw 1    ; Flags (0 = optionnal, 1 = required)
        dd bufferframeTagEnd - bufferframeTag   ; Size of the tag
        dd 640  ; Width
        dd 480  ; Height
        dd 32   ; Depth
    bufferframeTagEnd:

    align 8
headerEnd:
    dd 0                    ; Type (End tag)
    dd 8                    ; Size of this tag
    ; Multiboot2 header end