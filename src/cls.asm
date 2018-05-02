[BITS 16]
; [ORG 0x15E00]
jmp main

%include "src\const.inc"

;===============================================
; Entry point
;===============================================
main:
    push    ax
    mov     ax,INT_CLEAR_SCREEN
    int     70h
    pop     ax
    retf
