gdt32:
    .null:
        dq  0
    .code:
        dw  0xffff      ; segment length bits 0-15
        dw  0x0000      ; segment base bits 0-15

        db  0x00        ; segment base bits 16-23
        db  0b10011111  ; access byte (refer to docs online)
        db  0b11001111  ; flags (bits) + segment length bits 16-19
        db  0x00        ; segment base bits 24-31
    .data:
        dw  0xffff
        dw  0x0000

        db  0x00
        db  0b10010011
        db  0b11001111
        db  0x00
    .end:
    .descriptor:
        dw  .end - .null    ; gdt size
        dd  gdt32           ; gdt address