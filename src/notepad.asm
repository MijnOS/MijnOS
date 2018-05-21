[BITS 16]
; [ORG 0x15E00]
jmp main

%include "src\const.inc"
%define BUFFER_SIZE     1024
text_buffer times BUFFER_SIZE db 0              ; Buffer the user writes to when pressing a key
text_size   dw 0                                ; Number of characters currently in the buffer
opt_quit    db 0                                ; Should notepad quit/terminate?
opt_menu    db 0                                ; Should the menu be displayed
text_bbuf   db 08h, 20h, 08h, 0                 ; Instructions for backspace
file_str    db 'FILE: ', 0
file_buff   times 16 db 0                       ; Max size is 12 incl. ext, excl. zst
.length     dw ($-$$)                           ; Length of the buffer
.count      dw 0                                ; Number of written characters
cursor_pos  dw 0                                ; Cursor position
%define MENU_COLOR  070h


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
    mov     cl,byte [opt_menu]                  ; Opt. 1) Check if the menu is opened
    test    cl,cl
    jne     .h_menu

    mov     cl,byte [opt_quit]                  ; Opt. 2) Quit the editor?
    test    cl,cl
    jne     exit

    mov     ax,INT_KEYPRESS                     ; 1) Wait for a key.
    int     70h

.stage1:                                        ; Check the key
    cmp     ax,20h
    jb      .stage2
    cmp     ax,7Fh
    jae     .stage2
    call    np_simpleChar                       ; It's a simple character
    jmp     .loop

.stage2:                                        ; Opt. 2) Complex, control characters
    call    np_complexChar
    jmp     .loop

.h_menu:
    call    handle_menu
    jmp     .loop


; NOTE:
;   The dot is missing so we can exit from
;   anywhere within the program.
exit:
    pop     ax
    pop     si
    retf


;===============================================
; Stores the cursor position.
;===============================================
util_setCursor:
    mov     ax,INT_GET_CURSOR_POS
    int     70h
    mov     word [cursor_pos],dx
    ret


;===============================================
; Restores the cursor position.
;===============================================
util_getCursor:
    mov     dx,word [cursor_pos]
    mov     ax,INT_SET_CURSOR_POS
    int     70h
    ret


;===============================================
; Copies characters in the range of [0x20, 0x7F)
; into the buffer and displays them on screen.
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
; Control characters, these require special
; functions to handle properly.
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
; Remove the last character from the buffer; and
; removed the the respective character(s) from
; the screen.
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
; Set the menu open flag
;===============================================
np_keyEscape:
    mov     byte [opt_menu],1
    ret



;===============================================
; Menu + options
;===============================================
handle_menu:


.active:
    ; Move the cursor to the lower-left corner
    mov     dh,24       ; row
    mov     dl,0        ; column
    mov     ax,INT_SET_CURSOR_POS
    int     70h

    ; Print the menu prefix
    mov     bx,MENU_COLOR
    mov     cx,03Ah                             ; ':'
    mov     ax,INT_PRINT_COLORED
    int     70h

; Menu loop
.loop:
    mov     ax,INT_KEYPRESS                     ; 1) Wait for a key press
    int     70h

    ; Display the typed character
    mov     cx,ax
    ;mov     bx,MENU_COLOR
    ;mov     ax,INT_PRINT_COLORED
    ;int     70h

; Menu options can be caught here
    cmp     cx,KEY_UC_Q                         ; QUIT
    je      .m_quit
    cmp     cx,KEY_LC_Q
    je      .m_quit

    cmp     cx,KEY_UC_W                         ; WRITE
    je      .m_write    
    cmp     cx,KEY_LC_W
    je      .m_write

    cmp     cx,KEY_UC_O                         ; OPEN
    je      .m_open
    cmp     cx,KEY_LC_O
    je      .m_open

    cmp     cx,KEY_ESCAPE                       ; Close the menu
    je      .m_close

    jmp     .loop                               ; Default

; Write to filename
.m_write:
    ; TODO:
    call    fn_typing
    je      .loop
    jmp     .m_close

; Open from filename
.m_open:
    ; TODO:
    call    fn_typing
    je      .loop
    jmp     .m_close


; Quit the application
.m_quit:
    mov     byte [opt_quit],1

; Close the menu and quit
.m_close:
    mov     byte [opt_menu],0
    ret





;===============================================
; Loops till the filename has been written
;===============================================
fn_typing:
    mov     si,file_str
    mov     ax,INT_PRINT_STRING
    int     70h

.loop:
    mov     ax,INT_KEYPRESS
    int     70h

    cmp     ax,KEY_ENTER
    je      .action

    cmp     ax,KEY_ESCAPE
    je      .escape

    cmp     ax,KEY_BACKSPACE
    je      .back

    call    fn_append
    cmp     ax,0
    je      .continue

    mov     cx,ax
    mov     bx,MENU_COLOR
    mov     ax,INT_PRINT_COLORED
    int     70h

.continue:
    jmp     .loop

.back:
    mov     bx,word [file_buff.count]
    sub     bx,1
    cmp     bx,0
    jl      .loop                               ; Do nothing when there is nothing
    mov     byte [file_buff+bx],0
    mov     word [file_buff.count],bx
    mov     si,text_bbuf                        ; Remove the character from the screen
    mov     ax,INT_PRINT_STRING
    int     70h
    jmp     .loop

.escape:
    mov     byte [opt_menu],0

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
    push    bp
    mov     bp,sp
    sub     sp,2
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
    mov     dx,bx
    add     dx,1
    cmp     dx,word [file_buff.length]
    jae     .return

    mov     byte [file_buff+bx],al
    add     bx,1
    mov     word [file_buff.count],bx

    mov     word [bp-2],ax
    popa
    mov     ax,word [bp-2]
    mov     sp,bp
    pop     bp
    ret

; period, extension sep
.2:
    cmp     ax,KEY_PERIOD
    je      .append

.return:
    popa
    xor     ax,ax
    mov     sp,bp
    pop     bp
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
