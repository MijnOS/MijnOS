[BITS 16]
; [ORG 0x13E00]
jmp main

%include "src\const.inc"
%define BUFFER_SIZE     64
msg_cmd     db 'MijnOS CMD 2018-04-29', 0Dh, 0Ah, 0
cmd_buffer  times BUFFER_SIZE db 0
cmd_offset  dw 0
cmd_prefix  db '> ', 0
; name (8) + seperator (1) + extension (3) + terminator (1) = 13

back_buffer db 08h, 20h, 08h, 0


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

.init:
    mov     ax,INT_CLEAR_SCREEN
    int 	70h
    mov     si,msg_cmd
    mov     ax,INT_PRINT_STRING
    int     70h
    call    print_prefix

.keypress:
    mov     ax,INT_KEYPRESS
    int     70h
    cmp     ax,KEY_BACKSPACE
    ;je      .clear
    je      .backspace
    cmp     ax,KEY_ENTER
    je      .exec

.cmp0:                                          ; 0-9
    cmp     ax,KEY_0
    jb      .cmp1
    cmp     ax,KEY_9
    jbe     .simpleChar

.cmp1:                                          ; A-Z
    cmp     ax,KEY_UC_A
    jb      .cmp2
    cmp     ax,KEY_UC_Z
    jbe     .simpleChar

.cmp2:                                          ; a-z
    cmp     ax,KEY_LC_A
    jb      .cmp3
    cmp     ax,KEY_LC_Z
    ja      .cmp3
    sub     ax,20h                              ; toUpper
    jmp     .simpleChar

.cmp3:
    cmp     ax,KEY_SPACE
    je      .simpleChar
    cmp     ax,KEY_PERIOD
    je      .simpleChar

.continue:
    jmp     .keypress

.simpleChar:
    mov     dx,word [cmd_offset]
    cmp     dx,BUFFER_SIZE
    jae     .keypress

    ; Store the character in the buffer
    mov     bx,cmd_buffer
    add     bx,word [cmd_offset]
    mov     byte [bx],al

    ; Increment the offset
    add     word [cmd_offset],1

    ; Print the character to the screen
    mov     cx,ax
    mov     ax,INT_PRINT_CHAR
    int     70h
    jmp     .keypress

.clear:
    mov     ax,INT_CLEAR_SCREEN
    int 	70h
    call    print_prefix
    jmp     .keypress

; == SPECIAL KEYS ==============================
.backspace:
    mov     dx,word [cmd_offset]
    cmp     dx,0
    jle     .keypress

    ; Clear the last byte in the buffer
    push    bx                  
    mov     bx,cmd_buffer
    add     bx,dx
    mov     byte [bx],0
    pop     bx

    ; Decrement the offset
    sub     word [cmd_offset],1

    ; Replaces the last character on the screen
    ; with a space and decrements the pointer
    mov     si,back_buffer
    mov     ax,INT_PRINT_STRING
    int     70h
    jmp     .keypress

.exec:
    mov     word [cmd_offset],0
    mov     ax,INT_PRINT_NEWLINE
    int     70h
    call    print_prefix
    jmp     .keypress

    jmp     $



print_prefix:
    push    si
    push    ax
    mov     si,cmd_prefix
    mov     ax,INT_PRINT_STRING
    int     70h
    pop     ax
    pop     si
    ret
