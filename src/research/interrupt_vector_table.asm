BITS 16

;===========
; Start of the bootloader
;===========
start:
    mov     ax,07C0h                ; Set up 4K stack space after this bootloader
    add     ax,288                  ; (4096 + 512) / 16 bytes per paragraph
    mov     ss,ax
    mov     sp,4096

    mov     ax,07C0h                ; Set data segment to where we're loaded
    mov     ds,ax

    ; mov     ah,00h                  ; set video mode to graphics
    ; mov     al,10h
    ; int     10h

    call    print_ivt

    mov     si,text_string          ; Put string position into SI
    call    print_string            ; Call our string-printing routine

    jmp     $                       ; Jump here - infinite loop! 

    text_string db 0Dh, 0Ah, 'This is my cool new OS!', 0Dh, 0Ah, 0


;===========
; Prints a string onto the screen
;===========
print_string:                       ; Routine: output string in SI to screen
    mov     ah,0Eh                  ; int 10h 'print char' function

.repeat:
    lodsb                           ; Get character from string
    cmp     al,0
    je      .done                   ; If char is zero, end of string
    int     10h                     ; Otherwise, print it
    jmp     .repeat
 
.done:
    ret


;===========
; Display the contents of the Interrupt Vector Table
;===========
print_ivt:
    push    ax
    push    bx
    push    cx
    push    dx
    push    es

    mov     ax,0000h
    mov     es,ax                   ; IVT table segment
    mov     bx,ax                   ; IVT index offset

.pi_loop:
    mov     cx,[es:bx]              ; IVT[i].segment
    mov     dx,[es:bx+2]            ; IVT[i].offset

    mov     ax,cx                   ; print the segment
    shr     ax,8
    push    ax
    call    print_hex
    mov     ax,cx
    and     ax,0FFh
    push    ax
    call    print_hex
    add     sp,4

    mov     ax,dx                   ; print the offset
    shr     ax,8
    push    ax
    call    print_hex
    mov     ax,cx
    and     ax,0FFh
    push    ax
    call    print_hex
    add     sp,4

    ;push    ax                      ; increment the counter
    ;mov     ax,bx
    ;shr     ax,2                    ; Goes on a base of 4
    ;and     ax,7
    ;cmp     ax,0
    ;pop     ax
    ;jne     .pi_print

    ;mov     si,newline              ; seperate the lines in the table
    ;call    print_string

.pi_print:
    add     bx,4
    cmp     bx,0400h                ; 4 * 256 / 4 * 16 == 4 * 16*16
    jl      .pi_loop


.pi_done:
    pop     es
    pop     dx
    pop     cx
    pop     bx
    pop     ax
    ret

    newline db 0Dh, 0Ah, 0


;===========
; Print HEX character
;   void print_hex(byte);
;===========
print_hex:
    push    ax
    push    bx
    push    dx

    mov     bx,sp
    mov     dx,[ss:bx+8]

.ph_high:                           ; Display high-byte
    mov     ax,dx
    and     ax,0F0h
    shr     ax,4
    cmp     ax,10
    jl      .ph_high_digit

.ph_high_char:
    add     ax,37h
    jmp     .ph_high_done

.ph_high_digit:
    add     ax,30h

.ph_high_done:
    mov     ah,0Eh
    int     10h

.ph_low:                            ; Display low-byte
    mov     ax,dx
    and     ax,0Fh
    cmp     ax,10
    jl      .ph_low_digit

.ph_low_char:
    add     ax,37h
    jmp     .ph_low_done

.ph_low_digit:
    add     ax,30h

.ph_low_done:
    mov     ah,0Eh
    int     10h

.ph_done:
    pop     dx
    pop     bx
    pop     ax
    ret



times 510-($-$$) db 0               ; Pad remainder of boot sector with 0s
dw 0xAA55                           ; The standard PC boot signature 
