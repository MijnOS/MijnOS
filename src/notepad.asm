[BITS 16]
; [ORG 0x15E00]
jmp main

%include "src\const.inc"
%define BUFFER_SIZE     1024
text_buffer times BUFFER_SIZE db 0              ; Buffer the user writes to when pressing a key
text_size   dw 0                                ; Number of characters currently in the buffer
text_quit   db 0                                ; Should notepad quit/terminate?
text_menu   db 0                                ; Should the menu be displayed
text_bbuf   db 08h, 20h, 08h, 0                 ; Instructions for backspace
file_str    db 'FILE: ', 0
file_buff   times 16 db 0                       ; Max size is 12 incl. ext, excl. zst
.length     dw ($-$$)                           ; Length of the buffer
.count      dw 0                                ; Number of written characters

;===============================================
; Entry point
;===============================================
main:
    push    si
    push    ax

.init:
    mov     ax,INT_CLEAR_SCREEN
    int     70h                                 ; Ensure te screen is clear
    xor     dx,dx
    mov     ax,INT_SET_CURSOR_POS
    int     70h                                 ; Ensure we are top-left

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
    movzx   cx,byte [text_quit]                 ; Quit notepad
    test    cx,cx
    je      .loop

; NOTE:
;   The dot is missing so we can exit from
;   anywhere within the program.
exit:
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
    mov     byte [text_menu],0FFh

.active:
    ; Move the cursor to the lower-left corner
    mov     dh,24       ; row
    mov     dl,0        ; column
    mov     ax,INT_SET_CURSOR_POS
    int     70h

    ; Print a colored character
    mov     bx,070h
    mov     cx,03Ah
    mov     ax,INT_PRINT_COLORED
    int     70h

; Menu loop
.loop:
    mov     ax,INT_KEYPRESS
    int     70h
    mov     cx,ax
    mov     bx,070h
    mov     ax,INT_PRINT_COLORED
    int     70h

; Menu options can be caught here
    cmp     cx,KEY_UC_Q                         ; QUIT
    je      .quit
    cmp     cx,KEY_LC_Q
    je      .quit

    cmp     cx,KEY_UC_W                         ; WRITE
    je      .m_write    
    cmp     cx,KEY_LC_W
    je      .m_write

    cmp     cx,KEY_UC_O                         ; OPEN
    je      .m_open
    cmp     cx,KEY_LC_O
    je      .m_open

    cmp     cx,KEY_ESCAPE                       ; Close the menu
    je      .closeMenu

    jmp     .loop                               ; Default

.m_write:
    ; TODO:
    mov     si,file_str
    mov     ax,INT_PRINT_STRING
    int     70h
    jmp     .loop

.m_open:
    ; TODO:
    mov     si,file_str
    mov     ax,INT_PRINT_STRING
    int     70h
    jmp     .loop

.closeMenu:
    mov     byte [text_menu],0
    jmp     .return

.quit:
    mov     byte [text_quit],1
    jmp     .return

.return:
    ret



;===============================================
; Loops till the filename has been written
;===============================================
fn_typing:
    call    menu_clear

.loop:
    mov     si,file_str
    mov     ax,INT_PRINT_STRING
    int     70h

    mov     ax,INT_KEYPRESS
    int     70h

    cmp     ax,KEY_ENTER
    je      .action

    cmp     ax,KEY_ESCAPE
    je      .escape

    cmp     ax,KEY_BACKSPACE
    je      .back

    call    fn_append
    jne     .continue

.continue:
    jmp     .loop

.back:
    mov     bx,word [file_buff.count]
    sub     bx,1
    mov     byte [file_buff.count],0
    mov     word [file_buff.count],bx
    jmp     .loop

.escape:
    mov     byte [text_menu],0

.return:
    mov     ax,0FFFFh
    ret


.action:
    xor     ax,ax
    ret


;===============================================
; Appends the filename with the specified character
;===============================================
fn_append:
    pusha

; lowercase
.0:
    cmp     ax,KEY_LC_A
    jl      .1
    cmp     ax,KEY_LC_Z
    ja      .return     ; Invalid
    sub     ax,020h
    jmp     .append

; uppercase
.1:
    cmp     ax,KEY_UC_A
    jl      .2
    cmp     ax,KEY_UC_Z
    ja      .return

; append the character to the buffer
.append:
    mov     bx,word [file_buff.count]
    mov     byte [file_buff+bx],al
    add     bx,1
    mov     word [file_buff.count],bx
    popa
    xor     ax,ax
    ret

; period, extension sep
.2:
    cmp     ax,KEY_PERIOD
    je      .append

.return:
    popa
    mov     ax,0FFFFh
    ret



;===============================================
; Clears the menu bar
;===============================================
menu_str    times 80 db 020h
menu_clear:
    pusha

    mov     cx,79
.loop:
    ; Move the cursor to the lower-left corner
    mov     dh,24       ; row
    mov     dl,cl       ; column
    mov     ax,INT_SET_CURSOR_POS
    int     70h

    ; Print a colored character
    mov     cx,020h
    mov     ax,INT_PRINT_CHAR
    int     70h

    loop    .loop

    popa
    ret

;===============================================
; Zero fills the filename buffer
;===============================================
fn_clear:
    pusha
    mov     cx,word [file_buff.length]
.loop:
    mov     bx,cx
    sub     bx,1
    add     bx,file_buff
    mov     byte [bx],0
    loop    .loop
    popa
    ret



;===============================================
; Not currently implemented.
;===============================================
np_keyDelete:
    ret
