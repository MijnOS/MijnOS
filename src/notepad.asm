[BITS 16]
; [ORG 0x15E00]
jmp main

%include "src\const.inc"
%define BUFFER_SIZE     1024
text_buffer times BUFFER_SIZE db 0              ; Buffer the user writes to when pressing a key
text_size   dw 0                                ; Number of characters currently in the buffer
text_quit   dw 0                                ; Should notepad quit/terminate?
text_bbuf   db 08h, 20h, 08h, 0                 ; Instructions for backspace

;===============================================
; Entry point
;===============================================
main:
    push    si
    push    ax

.loop:
    mov     ax,INT_KEYPRESS
    int     70h

.stage1:                                        ; 1) Simple, human readable
    cmp     ax,20h
    jb      .stage2
    cmp     ax,7Fh
    jae     .stage2
    call    np_simpleChar
    jmp     .loop

.stage2:                                        ; 2) Complex, control characters
    call    np_complexChar

.continue:
    ; test
    xor     dx,dx
    mov     dh,5
    mov     ax,INT_SET_CURSOR_POS
    int     70h

    mov     ax,word [text_quit]
    test    ax,ax
    je      .loop

.exit:
    pop     ax
    pop     si
    retf



;===============================================
; [0x20, 0x7F)
;===============================================
np_simpleChar:
    mov     cx,word [text_size]
    cmp     cx,BUFFER_SIZE
    jae     .return

.insert:
    mov     di,text_buffer
    add     di,cx
    mov     word [di],ax
    add     word [text_size],1

.print:
    mov     cx,ax
    mov     ax,INT_PRINT_CHAR
    int     70h

.return:
    ret



;===============================================
; Control characters
;===============================================
np_complexChar:

.0:
    cmp     ax,08h      ; KEY_BACKSPACE
    jne     .1
    call    np_keyBackspace
    jmp     .return

.1:
    cmp     ax,09h      ; KEY_TAB
    jne     .2
    call    np_keyTab
    jmp     .return

.2:
    cmp     ax,0Ah      ; KEY_NEWLINE
    jne     .3
    call    np_keyNewline
    jmp     .return

.3:
    cmp     ax,0Dh      ; KEY_RETURN
    jne     .4
    call    np_keyNewline
    jmp     .return

.4:
    cmp     ax,1Bh      ; KEY_ESCAPE
    jne     .5
    call    np_keyEscape
    jmp     .return

.5:
    cmp     ax,7Fh      ; KEY_DELETE
    jne     .return
    call    np_keyDelete

.return:
    ret



;===============================================
; Remove the last character from the buffer.
;===============================================
np_keyBackspace:
    mov     cx,word [text_size]
    cmp     cx,0
    jbe     .return

    mov     di,text_buffer
    add     di,cx
    sub     di,1

.check:
    mov     al,byte [di]
    cmp     al,09h      ; KEY_TAB
    je      .tab
    cmp     al,0Ah      ; \r\n
    je      .newline

    mov     si,text_bbuf
    mov     ax,INT_PRINT_STRING
    int     70h

.insert:
    mov     word [di],0
    sub     word [text_size],1

.return:
    ret

; Tabs are displayed using 4-spaces
.tab:
    mov     cx,4

.loop:
    mov     si,text_bbuf
    mov     ax,INT_PRINT_STRING
    int     70h
    loop    .loop

    jmp     .insert

; New lines consist of \r\n
.newline:
    mov     word [di],0
    sub     word [text_size],1

    mov     si,text_bbuf
    mov     ax,INT_PRINT_STRING
    int     70h

    sub     di,1
    mov     al,byte [di]                        ; We must check the input,
    cmp     al,0Dh                              ; as some files may only use \n
    jne     .return

    mov     word [di],0
    sub     word [text_size],1

    mov     si,text_bbuf
    mov     ax,INT_PRINT_STRING
    int     70h

    jmp     .return




;===============================================
; TABs can be insert like a normal character.
;===============================================
np_keyTab:
    mov     cx,word [text_size]
    cmp     cx,BUFFER_SIZE
    jae     .return

.insert:
    mov     di,text_buffer
    add     di,cx
    mov     word [di],09h
    add     word [text_size],1

    mov     cx,4
.print:
    push    cx
    mov     cx,20h
    mov     ax,INT_PRINT_CHAR
    int     70h
    pop     cx
    loop    .print

.return:
    ret



;===============================================
; New lines are done \r\n style.
;===============================================
np_keyNewline:
    mov     cx,word [text_size]
    mov     dx,BUFFER_SIZE-1
    cmp     cx,dx
    jae     .return

.insert:
    mov     di,text_buffer
    add     di,cx
    mov     word [di+0],0Dh
    mov     word [di+1],0Ah
    add     word [text_size],2

.print:
    mov     si,di
    mov     ax,INT_PRINT_STRING
    int     70h

.return:
    ret



;===============================================
; Menu + options
;===============================================
np_keyEscape:
    mov     word [text_quit],1
    ret



;===============================================
; Not currently implemented.
;===============================================
np_keyDelete:
    ret
