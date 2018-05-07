[BITS 16]
; [ORG SEG_PROGRAM]
jmp main

%include "src\const.inc"

;===============================================
; Entry point
;===============================================
main:
    pusha

    mov     bx,1
    mov     cx,2
    mov     dx,3
    mov     si,4
    mov     di,5


    mov     ax,7FFFh
    int     70h

    popa
    retf
