[BITS 16]


;===============================================
; Entry point
;===============================================
cmd:
    mov     si,msg_cmd
    call    print

.keypress:
    mov     ah,00h
    int 	16h
    movzx   ax,al
    call    print_hex
    call    print_newline
    jmp     .keypress

    jmp     $

;===========
; STRINGS
;===========
msg_cmd     db "CMD", 0Dh, 0Ah, 0


%include "src\kernel\std.inc"
