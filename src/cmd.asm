[BITS 16]
; [ORG 0x13E00]
jmp main

%include "src\const.inc"
msg_cmd     db 'CMD program', 0Dh, 0Ah, 0

;===============================================
; Entry point
;===============================================
main:
    mov     ax,cs
    mov     ds,ax
    mov     es,ax
    add     ax,100h
    mov     ss,ax
    mov     sp,1000h

    mov     si,msg_cmd
    mov     ax,INT_PRINT_STRING
    int     70h

.keypress:
    mov     ax,INT_KEYPRESS
    int     70h
    cmp     ax,KEY_BACKSPACE
    je      .clear

    mov     cx,ax
    mov     ax,INT_PRINT_HEX
    int     70h

    jmp     .keypress

.clear:
    mov     ax,INT_CLEAR_SCREEN
    int 	70h
    jmp     .keypress

    jmp     $
