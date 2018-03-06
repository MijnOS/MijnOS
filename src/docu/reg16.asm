[BITS 16]
[MAP ALL reg16.map]

start:
    mov     bx,0
    mov     dx,word [bx]
    ret

segments:
    mov     ax,0
    mov     fs,ax
    ret
