[BITS 16]
; [ORG 0x15E00]
jmp main

%include "src\const.inc"
%define BUFFER_SIZE     512
text_buffer times BUFFER_SIZE db 0              ; Buffer the user writes to when pressing a key
text_double times BUFFER_SIZE db 0              ; Double buffer for redraws
text_size   dw 0                                ; Number of characters currently in the buffer
opt_quit    db 0                                ; Should notepad quit/terminate?
opt_menu    db 0                                ; Should the menu be displayed
text_bbuf   db 08h, 20h, 08h, 0                 ; Instructions for backspace
file_str    db 'FILE: ', 0
file_buff   times 16 db 0                       ; Max size is 12 incl. ext, excl. zst
.length     dw ($-file_buff)                    ; Length of the buffer
.count      dw 0                                ; Number of written characters
cursor_pos  dw 0                                ; Cursor position
%define MENU_COLOR  070h
text_ferr   dw 0


;===============================================
; Entry point
;===============================================
main:
    push    bp
    mov     bp,sp

; initialize the program
.init:
    mov     ax,INT_CLEAR_SCREEN
    int     70h                                 ; Ensure te screen is clear
    xor     dx,dx
    mov     ax,INT_SET_CURSOR_POS
    int     70h                                 ; Ensure we are top-left

; programs main loop
.loop:
    mov     cl,byte [opt_menu]                  ; Opt. 1) Check if the menu is opened
    test    cl,cl
    jne     .h_menu

    mov     cl,byte [opt_quit]                  ; Opt. 2) Quit the editor?
    test    cl,cl
    jne     exit

    mov     ax,INT_KEYPRESS                     ; 1) Wait for a key.
    int     70h
    call    np_drawChar

    jmp     .loop                               ; continue the program loop

; menu handling code
.h_menu:
    call    handle_menu
    jmp     .loop


; NOTE:
;   The dot is missing so we can exit from
;   anywhere within the program.
exit:
    mov     sp,bp
    pop     bp
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
; Draws the character to the screen if necessary
;===============================================
np_drawChar:
    cmp     ax,20h                              ; Opt. 1) Complex, control characters
    jb      np_complexChar
    cmp     ax,7Fh                              ; Opt. 2) Complex, language characters
    jae     np_complexChar
    jmp     np_simpleChar                       ; Opt. 3) It's a simple character


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
    call    util_setCursor
    ret



;===============================================
; Closes the menu and redraws all the contents
;===============================================
np_closeMenu:
    push    ax
    mov     byte [opt_menu],0       ; always hide the menu
    
    mov     al,byte [opt_quit]
    test    al,al
    jne     .return

.redraw:
    call    util_getCursor          ; only execute when not quiting
    call    np_refillScreen

.return:
    pop     ax
    ret


;===============================================
; Refills the screen with the data from the
; text buffer.
;===============================================
np_refillScreen:
    pusha

    ; Clear the screen
    mov     ax,INT_CLEAR_SCREEN
    int     70h

    ; Move the cursor back to 0,0
    xor     dx,dx
    mov     ax,INT_SET_CURSOR_POS
    int     70h

    ; Copy the buffer contents
.copy:
    mov     cx,word [text_size]
    mov     si,text_buffer
    mov     di,text_double

.copy_loop:
    mov     al,byte [ds:si]
    mov     byte [ds:di],al
    add     si,1
    add     di,1
    sub     cx,1
    jne     .copy_loop

    ; Zero-terminate the output
    mov     byte [ds:di],0

    ; Redraw the file's contents
    mov     cx,word [text_size]
    mov     word [text_size],0
    mov     si,text_double
.redraw:
    movzx   ax,byte [ds:si]
    ;test    ax,ax
    ;je      .return
    push    si
    push    cx
    call    np_drawChar
    pop     cx
    pop     si
    add     si,1
    sub     cx,1
    jne     .redraw

.return:
    popa
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
    ;je      exit
    cmp     cx,KEY_LC_Q
    je      .m_quit
    ;je      exit

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
    test    ax,ax
    je      .loop
    ;call    np_writeFile
    jmp     .m_close

; Open from filename
.m_open:
    ; TODO:
    call    fn_typing
    test    ax,ax
    je      .loop
    ;call    np_loadFile
    jmp     .m_close


; Quit the application
.m_quit:
    mov     byte [opt_quit],1

; Close the menu and quit
.m_close:
    call    np_closeMenu
    ret


;===============================================
; Loads a file from the medium.
;===============================================
np_loadFile:
    pusha

    ; file_name
    ;mov    ds,ds
    mov     si,file_buff

    ; file_size
    push    dx
    push    ax
    mov     ax,INT_FILE_SIZE
    int     70h
    mov     word [text_size],dx
    pop     ax
    pop     dx

    ; file_data
    mov     bx,ds
    mov     es,bx
    mov     di,text_buffer

    ; clear the text buffer
    ;push    ax
    ;push    di
    ;push    cx
    ;mov     al,0
    ;mov     cx,BUFFER_SIZE
    ;mov     di,text_buffer
    ;cld
    ;rep     stosb
    ;pop     di
    ;pop     cx
    ;pop     ax

    ; load file
    mov     cx,text_ferr
    mov     ax,INT_LOAD_FILE
    int     70h
    mov     ax,word [text_ferr]

    ; check for errors
    test    ax,ax
    jne     .error

.succes:
    ; TODO: update the gui
    ;call    np_refillScreen
    jmp     .return

.error:
    mov     byte [opt_quit],1                   ; Terminate on load error
    jmp     .return

.return:
    mov     byte [opt_menu],0                   ; Hide the menu
    popa
    ret


;===============================================
; Writes a file from the medium.
;===============================================
np_writeFile:
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

.limit:
    mov     cx,word [file_buff.count]
    add     cx,1
    cmp     cx,word [file_buff.length]
    jae     .continue

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
    call    np_closeMenu

.return:
    mov     ax,0FFFFh
    ret

.action:
    call    np_closeMenu
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

.init:
    mov     word [bp-2],0

; period, extension sep
.period:
    cmp     ax,KEY_PERIOD
    je      .append     ; character is [.]

; numbers
.0:
    cmp     ax,KEY_0
    jl      .return
    cmp     ax,KEY_9
    jle     .append     ; character is [0-9]

; uppercase
.1:
    cmp     ax,KEY_UC_A
    jl      .return
    cmp     ax,KEY_UC_Z
    jle     .append     ; character is [A-Z]

; lowercase
.2:
    cmp     ax,KEY_LC_A
    jl      .return
    cmp     ax,KEY_LC_Z
    ja      .return     ; invalid character
    sub     ax,020h     ; convert [a-z] to [A-Z]
    jmp     .append

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

; only reaches here when successful
.success:
    mov     word [bp-2],ax

; return to caller
.return:
    popa
    mov     ax,word [bp-2]
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
