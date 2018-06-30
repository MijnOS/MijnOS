[BITS 16]
; [ORG SEG_KERNEL]
jmp near kernel

%include "src/const.inc"

msg_success db "Kernel has been loaded...", 0Dh, 0Ah, 0
cmd_bin     db 'CMD     BIN', 0
cmd_error   db 'Could not load CMD.bin', 0Dh, 0Ah, 0

kernel_var  dw 512

test_err    db 'Test failed', 0Dh, 0Ah, 0
test_var    times 32 db 0

test_name   db 'ABCDEFGHEXT',0
test_guid   db 'DMMY10  TXT',0
test_cfile  db 'DMMY11  TXT',0
test_copy   db 'COPY01  TXT',0


;===============================================
; Entry point of the kernel module.
;===============================================
kernel:
    mov     ax,cs
    mov     ds,ax
    mov     es,ax
    mov     gs,ax       ; NOTE: This is used for interrupt calls
    add     ax,400h     ; 16kb
    mov     ss,ax
    mov     sp,4000h    ; 16kb

    ; Indicator that we lsuccesfully loaded the kernel
    mov     si,msg_success
    call    print

;    mov     bx,ds
;    mov     es,bx
;    mov     di,test_var
;
;    mov     ax,2                    ; CMD.BIN
;    call    fat_rootGetEntry
;    test    ax,ax
;    jne     .skip
;
;    mov     si,test_var
;    mov     cx,8
;    call    printn
;
;    mov     byte [ds:si],041h       ; AMD.BIN
;    mov     ax,2
;    call    fat_rootSetEntry
;    test    ax,ax
;    je      .clear
;
;.skip:
;    mov     si,test_err
;    call    print

;.write_test:
;    push    es
;    push    ds

;    mov     cx,32
;    sub     sp,cx
;    mov     bp,sp
;    lea     di,[bp]
;    mov     si,test_name
;    call    fat_writeFile

;    pop     ds
;    pop     es


; Regular operations
;.clear:
;    call    register_interrupts
;    call    exec_cmd

.tests:
    jmp     .test_new_writeData
    ;jmp     .keypress


.test_old_getRootEntry:
    mov     bx,ds
    mov     es,bx
    mov     di,test_var

    mov     ax,2                    ; CMD.BIN
    call    fat_rootGetEntry
    call    print_hex
    call    print_newline

    mov     si,test_var
    mov     cx,11
    call    printn
    call    print_newline

.test_new_calc:
    mov     ax,11
    call    fat_calcClusters
    call    print_hex
    call    print_newline

.test_new_alloc:
    mov     ax,027h
    mov     cx,5
    call    fat_allocClusters
    call    print_hex
    call    print_newline
    mov     ax,cx
    push    ax
    call    print_hex
    call    print_newline
    pop     ax
    jmp     .test_new_reloc

.test_new_free:
    mov     ax,029h
    call    fat_freeClusters
    call    print_hex
    call    print_newline

.test_new_write:
    mov     ax,028h
    call    fat_getLastFileCluster
    call    print_hex
    call    print_newline

.test_new_reloc:
    push    ax
    mov     cx,ax
    mov     ax,2                    ; SHRINK: not implemented so ok :-)
    call    fat_relocClusters2
    call    print_hex
    call    print_newline
    pop     ax

    mov     cx,ax
    mov     ax,7                    ; GROW: successful
    call    fat_relocClusters2
    call    print_hex
    call    print_newline

.test_new_getEntryId:
    mov     si,test_guid
    call    fat_getEntryId
    push    ax

    call    print_hex
    call    print_newline

    mov     si,test_guid
    call    print
    call    print_newline

.test_new_fileResize:
    mov     si,test_guid
    mov     cx,1000
    pop     ax
    call    fat_fileResize
    call    print_hex
    call    print_newline

.test_new_createFile:
    mov     si,test_cfile
    call    fat_createFile
    call    print_hex
    call    print_newline
    jmp     .keypress

.test_new_readOrCreate:
    push    bp
    mov     bp,sp
    sub     sp,32

    mov     bx,ds
    mov     es,bx
    lea     di,[bp-32]
    ;mov     si,test_guid
    mov     si,test_cfile

    call    fat_fileReadOrCreateEntry
    call    print_hex
    call    print_newline

    test    ax,ax
    jne     .test_new_roc_return

    lea     si,[bp-32]
    mov     cx,11
    call    printn
    call    print_newline

.test_new_roc_return:
    mov     sp,bp
    pop     bp
    jmp     .keypress

.test_new_writeFile:
    push    01600h      ; file size
    push    0           ; [ds:0]
    push    ds          ;   kernel address in memory
    push    test_copy   ; [ds:test_copy]
    push    ds          ;   name of the file to write
    call    fat_writeFile2
    add     sp,10

    call    print_hex
    call    print_newline
    jmp     .keypress

.test_new_writeData:
    push    28h         ; DMMY10.TXT / 0x8E00
    mov     ax,word [data_size]
    push    ax
    push    data_buff
    push    ds
    call    fat_writeData
    add     sp,8

    call    print_hex
    call    print_newline
    jmp     .keypress

.keypress:
    mov     ah,00h
    int     16h
    movzx   ax,al
    call    print_hex
    call    print_newline
    jmp     .keypress

    jmp     $



;===============================================
; [Internal]
;   Starts the execution of the CMD program.
;===============================================
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
; [Internal]
;   Registers the interrupts.
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
; [External]
;   Set ax and than call int 70h. The input and
;   output differs per request made.
;===============================================
kernel_interrupts:
    push    gs
    push    bx
    mov     bx,SEG_KERNEL                       ; Fail-safe
    mov     gs,bx
    pop     bx

    cmp     ax,INT_FILE_SIZE
    je      .getFileSize

    cmp     ax,INT_LOAD_FILE
    je      .loadFile
    cmp     ax,INT_WRITE_FILE
    je      .writeFile
    cmp     ax,INT_EXEC_PROGRAM
    je      .execProgram
    cmp     ax,INT_GPU_GRAPHICS
    je      .gpuGraphics
    cmp     ax,INT_GPU_TEXT
    je      .gpuText

    cmp     ax,INT_KEYPRESS
    je      .getChar
    cmp     ax,INT_GET_CURSOR_POS
    je      .getCursorPos
    cmp     ax,INT_SET_CURSOR_POS
    je      .setCursorPos

    cmp     ax,INT_DRAW_PIXEL
    je      .drawPixel
    cmp     ax,INT_DRAW_BUFFER
    je      .drawBuffer

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
    cmp     ax,INT_PRINT_COLORED
    je      .printColored

.return:
    pop     gs
    iret


.getFileSize:
    jmp     .return

; short ax loadFile( void * es:di , char * ds:si )
; short ax loadFile( void * dest, char * error )
.loadFile:
    push    bx
    mov     bx,cx
    call    fat_loadFile
    mov     word [ds:bx],ax
    pop     bx
    jmp     .return

; Writes to a file
; [ds:si]   File_name
; [es:di]   File_data
; cx        File_size
.writeFile:
    call    fat_writeFile
    jmp     .return

; void ax execProgram( char * ds:si )
.execProgram:
    ; NOTE:
    ;   Impossible as is, thus cmd should load
    ;   it to the proper address and boot from
    ;   that point onwards.
    jmp     .return

; void func(void)
.gpuGraphics:
    push    ax
    mov     ah,0
    mov     al,13h      ; VGA / 16-colors / 320x200
    int     10h
    pop     ax
    jmp     .return

; void func(void)
.gpuText:
    push    ax
    mov     ah,0
    mov     al,03h      ; Text / 16-colors / 80x25
    int     10h
    pop     ax
    jmp     .return

; short ax getChar( void )
.getChar:
    mov     ah,00h
    int     16h
    movzx   ax,al
    jmp     .return

; ax, cx, dx
.getCursorPos:
    mov     bh,0
    mov     ah,3
    int     10h
    jmp     .return

; NULL
.setCursorPos:
    mov     ah,2
    mov     bh,0
    ;mov     dh,byte [row]
    ;mov     dl,byte [column]
    int     10h
    jmp     .return

; ax = INT_DRAW_PIXEL
; bx = color
; cx = x-pos
; dx = y-pos
.drawPixel:
    push    ax
    push    bx
    mov     ah,0Ch
    mov     al,bl
    xor     bx,bx
    int     10h
    pop     bx
    pop     ax
    jmp     .return

; ax = INT_DRAW_BUFFER
; es:bx = source buffer
.drawBuffer:            ; TODO:
    pusha
    push    ds
    push    es

    ; Offsets
    mov     si,bx
    xor     di,di

    ; Setup the new segments
    mov     bx,es       ; Source buffer
    mov     ds,bx
    mov     bx,0A000h   ; Video memory
    mov     es,bx

    ; The count
    mov     cx,(160*100)        ; All the pixels
    rep movsb

    pop     es
    pop     ds
    popa
    jmp     .return


; void clearScreen( void )
.clearScreen:
    push    ax
    mov     al,3
    mov     ah,0
    int     10h
    pop     ax
    jmp     .return     ; Cleared using video reset

;    push    ax          ; Clearing using \r\n
;    push    bx
;    xor     ax,ax
;    xor     bx,bx
;    mov     bh,0
;    mov     ah,2
;    mov     dh,24       ; HEIGHT: 25-characters
;    mov     dl,79       ; WIDTH : 80-characters
;    int     10h
;    pop     bx
;    pop     ax
;    push    cx
;    mov     cx,25
;.continue:
;    call    print_newline
;    loop    .continue
;    pop     cx
;    jmp     .return

; void printString( char * ds:si )
.printString:
    call    print
    jmp     .return

; void printHex( short cx )
.printHex:
    mov     ax,cx
    call    print_hex
    jmp     .return

; void printChar( char cl )
.printChar:
    pusha
    mov     bh,0
    mov     bl,7
    mov     ax,cx
    call    print_char
    popa
    jmp     .return

; void printNewLine( void )
.printNewLine:
    call    print_newline
    jmp     .return

; void printNString( char * ds:si, short cx )
.printNString:
    call    printn
    jmp     .return

.printColored:
    pusha
    mov     ax,cx
    mov     cx,1
    mov     bh,0
    call    print_colored
    popa
    jmp     .return


;===========
; DEPENDENCIES
;===========
%include "src/kernel/std.inc"
%include "src/kernel/fat12.inc"
%include "src/kernel/tests.inc"


;===========
; test_data
;===========
;data_buff   db 'This is a test buffer', 0
data_buff   times 514 db 1
data_size   dw $-data_buff
