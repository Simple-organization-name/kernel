BITS 16
jmp 07c0h:WORD __START__

__START__:
  mov ax, cs
  mov ss, ax
  xor sp, sp
  push cs
  pop ds
  push WORD 0b800h
  pop es

  mov ax, 03h
  int 10h



  ;^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^     
  ; \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
  ;  v   v   v   v   v   v   v   v   v   v   v   v   v   v   v   v   v

  ;Check APM service is present
  mov BYTE [error], 'A'  

  mov ax, 5300h
  xor bx, bx
  int 15h
  jc .err
  inc BYTE [error]
  cmp bx, 504dh
  jne .err

  ;^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^     
  ; \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
  ;  v   v   v   v   v   v   v   v   v   v   v   v   v   v   v   v   v

  ;Connect RM interface, compatibility mode APM 1.0
  inc BYTE [error]

  mov ax, 5301h
  xor bx, bx
  int 15h
  jc .err

  ;^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^     
  ; \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
  ;  v   v   v   v   v   v   v   v   v   v   v   v   v   v   v   v   v

  ;Switch to APM 1.1+
  inc BYTE [error]

  mov ax, 530eh
  xor bx, bx
  mov cx, 0101h
  int 15h
  jc .err
  inc BYTE [error]
  cmp al, 01h
  jb .err

  ;^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^     
  ; \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
  ;  v   v   v   v   v   v   v   v   v   v   v   v   v   v   v   v   v

  ;Enable APM
  inc BYTE [error]

  mov ax, 5308h
  mov bx, 01h
  mov cx, 01h
  int 15h
  jc .err

  ;^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^     
  ; \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
  ;  v   v   v   v   v   v   v   v   v   v   v   v   v   v   v   v   v

  ;Engage APM
  inc BYTE [error]

  mov ax, 530fh
  mov bx, 01h
  mov cx, 01h
  int 15h
  jc .err

  ;^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^     
  ; \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
  ;  v   v   v   v   v   v   v   v   v   v   v   v   v   v   v   v   v

  ;Shutdown
  inc BYTE [error]

  mov ax, 5307h
  mov bx, 01h
  mov cx, 03h
  int 15h
  jc .err

jmp .end 

.err:
  xor di, di
  mov ah, 09h
  mov al, BYTE [error]
  stosw

.end:
  cli
  hlt

  error db 0

  TIMES 505-($-__START__) db 0
  dw 0aa55h