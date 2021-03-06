%define test 2
[BITS 16]

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

    mov     si,msg_boot             ; Put string position into SI
    call    print_string            ; Call our string-printing routine

    jmp     $                       ; Jump here - infinite loop! 

    msg_boot db 'Booting MijnOS...', 0Dh, 0Ah, 0


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


times 510-($-$$) db 0               ; Pad remainder of boot sector with 0s
dw 0xAA55                           ; The standard PC boot signature 
