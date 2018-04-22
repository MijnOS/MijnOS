
;===============================================
; Prints a random string to the screen.
;===============================================
print_string:
    pusha
    mov     ah,0Eh

.loop:
    lodsb
    cmp     al,0
    je      .done
    int     10h
    jmp     .loop

.done:
    popa
    ret



;===============================================
; Prints a new line.
;===============================================
print_newline:
    push    ax
    mov     ah,0Eh
    mov     al,0Dh
    int     10h
    mov     al,0Ah
    int     10h
    pop     ax
    ret


;===============================================
; Prints a HEX byte onto the screen.
;===============================================
print_hex:
    pusha

    mov     cx,ax       ; Swap the bits for proper output
    xor     dx,dx

    shl     ax,4
    and     ax,0F00h
    or      dx,ax

    mov     ax,cx
    shr     ax,4
    and     ax,0F0h
    or      dx,ax

    mov     ax,cx
    shr     ax,12
    or      dx,ax

    mov     ax,cx
    shl     ax,12
    or      dx,ax

    mov     ax,dx       ; Swapped result is now in ax

    mov     bh,0        ; Teletype
    mov     bl,0Fh

    mov     cx,4        ; 2-bytes

    ; Minimum is 1-byte
.loop:
    and     ax,0Fh

    cmp     al,10
    jl      .low
    add     ax,07h                              ; 41h - 30h - 0Ah = 07h

.low:
    add     ax,30h
    mov     ah,0Eh
    int     10h

    mov     ax,dx       ; Shift and store
    shr     ax,4
    mov     dx,ax

    loop    .loop

.done:
    popa
    ret
