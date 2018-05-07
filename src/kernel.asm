[BITS 16]
; [ORG SEG_KERNEL]
jmp near kernel

%include "src/const.inc"

msg_success db "Kernel has been loaded...", 0Dh, 0Ah, 0
cmd_bin     db 'CMD     BIN', 0
cmd_error   db 'Could not load CMD.bin', 0Dh, 0Ah, 0

kernel_var  dw 512

;===============================================
; Entry point of the kernel module.
;===============================================
kernel:
    mov     ax,cs
    mov     ds,ax
    mov     es,ax
    add     ax,400h     ; 16kb
    mov     ss,ax
    mov     sp,4000h    ; 16kb

    ; Indicator that we lsuccesfully loaded the kernel
    mov     si,msg_success
    call    print

    ;call    fat_findEmptyCluster
    ;call    print_hex

    call    register_interrupts
    call    exec_cmd

;.keypress:
;    mov     ah,00h
;    int     16h
;    movzx   ax,al
;    call    print_hex
;    call    print_newline
;    jmp     .keypress

    jmp     $




exec_cmd:
    push    ds
    push    es
    push    si
    push    di
    push    bx

    ;mov     ds,ds
    mov     si,cmd_bin

    mov     bx,SEG_CMD
    mov     es,bx
    xor     di,di

    call    fat_loadFile
    test    ax,ax
    jne     .error

.success:
    call    SEG_CMD:0
    jmp     .return

.error:
    mov     si,cmd_error
    call    print

.return:
    pop     bx
    pop     di
    pop     si
    pop     es
    pop     ds
    ret



;===============================================
; Registers the interrupts.
;===============================================
register_interrupts:
    push    si

    mov     si,kernel_interrupts
    mov     ax,70h
    call    set_interrupt

    pop     si
    ret

set_interrupt:
    push    es
    push    bx

    mov     bx,0        ; Segment ZERO
    mov     es,bx

    mov     bx,ax
    shl     bx,2

    mov     word [es:bx],si         ; Function
    mov     word [es:bx+2],cs       ; Segment

    pop     bx
    pop     es
    ret


;===============================================
; Set ax and than call int 70h. The input and
; output differs per request made.
;===============================================
kernel_interrupts:
    cmp     ax,INT_LOAD_FILE
    je      .loadFile
    cmp     ax,INT_EXEC_PROGRAM
    je      .execProgram

    cmp     ax,INT_KEYPRESS
    je      .getChar
    cmp     ax,INT_GET_CURSOR_POS
    je      .getCursorPos
    cmp     ax,INT_SET_CURSOR_POS
    je      .setCursorPos

    cmp     ax,INT_CLEAR_SCREEN
    je      .clearScreen
    cmp     ax,INT_PRINT_STRING
    je      .printString
    cmp     ax,INT_PRINT_HEX
    je      .printHex
    cmp     ax,INT_PRINT_CHAR
    je      .printChar
    cmp     ax,INT_PRINT_NEWLINE
    je      .printNewLine
    cmp     ax,INT_PRINTN_STRING
    je      .printNString


    cmp     ax,7FFFh
    je      .testFunction

    iret


; short ax loadFile( void * es:di , char * ds:si )
.loadFile:
    push    bx
    mov     bx,cx
    call    fat_loadFile
    mov     word [ds:bx],ax
    pop     bx
    iret

; void ax execProgram( char * ds:si )
.execProgram:
    ; NOTE:
    ;   Impossible as is, thus cmd should load
    ;   it to the proper address and boot from
    ;   that point onwards.
    iret

; short ax getChar( void )
.getChar:
    mov     ah,00h
    int     16h
    movzx   ax,al
    iret

; ax, cx, dx
.getCursorPos:
    mov     bh,0
    mov     ah,3
    int     10h
    iret

; NULL
.setCursorPos:
    mov     ah,2
    mov     bh,0
    ;mov     dh,byte [row]
    ;mov     dl,byte [column]
    int     10h
    iret


; void clearScreen( void )
.clearScreen:
    push    ax
    push    bx
    xor     ax,ax
    xor     bx,bx
    mov     bh,0
    mov     ah,2
    mov     dh,24       ; HEIGHT: 25-characters
    mov     dl,79       ; WIDTH : 80-characters
    int     10h
    pop     bx
    pop     ax
    push    cx
    mov     cx,25
.continue:
    call    print_newline
    loop    .continue
    pop     cx
    iret

; void printString( char * ds:si )
.printString:
    call    print
    iret

; void printHex( short cx )
.printHex:
    mov     ax,cx
    call    print_hex
    iret

; void printChar( char cl )
.printChar:
    mov     ax,cx
    call    print_char
    iret

; void printNewLine( void )
.printNewLine:
    call    print_newline
    iret

; void printNString( char * ds:si, short cx )
.printNString:
    call    printn
    iret



.testFunction:
    push    ax
    push    bx

    ; ax | 7FFF
    ; bx | 
    ; cx | 
    ; dx | 
    ; si | ?
    ; di | ?
    ; ss | 16E0
    ; ds | 15E0
    ; es | 15E0

    ; ax
    call    print_hex
    call    print_newline

    ; bx
    mov     ax,bx
    call    print_hex
    call    print_newline

    ; cx
    mov     ax,cx
    call    print_hex
    call    print_newline

    ; dx
    mov     ax,dx
    call    print_hex
    call    print_newline

    ; si
    mov     ax,si
    call    print_hex
    call    print_newline

    ; di
    mov     ax,di
    call    print_hex
    call    print_newline

    ; ss
    mov     ax,ss
    call    print_hex
    call    print_newline

    ; ds
    mov     ax,ds
    call    print_hex
    call    print_newline

    ; es
    mov     ax,es
    call    print_hex
    call    print_newline

    ; debug (0)
    mov     ax,fat_getDebug1     ; 0x735
    call    print_hex
    call    print_newline

    ; debug (1)
    call    fat_getDebug1        ; 0x003 (?)
    call    print_hex
    call    print_newline

    ; debug (2)
    call    fat_getDebug1
    mov     bx,ax               ; 512
    mov     ax,word [bx]
    call    print_hex
    call    print_newline

    ; debug (3)
    call    fat_getDebug2        ; 512
    call    print_hex
    call    print_newline

    mov     ax,kernel_var
    call    print_hex
    call    print_newline

    mov     ax,word [kernel_var]
    call    print_hex
    call    print_newline

    pop     bx
    pop     ax
    iret



;===========
; DEPENDENCIES
;===========
%include "src/kernel/std.inc"
%include "src/kernel/fat12.inc"
%include "src/kernel/tests.inc"
