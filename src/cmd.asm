[BITS 16]


;===============================================
; Entry point
;===============================================
cmd:
    mov     si,msg_cmd
    call    print

    jmp     $


;===========
; void print(const char *si)
;   @param si A pointer to the string to print.
;===========
print:
    mov     ah,0Eh

.repeat:
    lodsb
    cmp     al,0
    je      .return
    int     10h
    jmp     .repeat

.return:
    ret

;===========
; STRINGS
;===========
msg_cmd     db "CMD" ,0Dh,0Ah,0
